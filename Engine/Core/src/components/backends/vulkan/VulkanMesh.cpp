#include "VulkanMesh.hpp"
#include "components/Mesh.hpp"
#include "glm/fwd.hpp"

#include <SDL3/SDL.h>

// debug
#include <cstdint>
#include <cstring>
#include <iostream>

namespace vex {
    VulkanMesh::VulkanMesh(VulkanContext& context) : m_r_context(context) {
        log("VulkanMesh created");
    }

    VulkanMesh::~VulkanMesh() {
        log("Destroying VulkanMesh");
        vkDeviceWaitIdle(m_r_context.device);

        for (auto& submesh : m_submeshBuffers) {
            if (submesh.vertexBuffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(m_r_context.allocator, submesh.vertexBuffer, submesh.vertexAlloc);
                submesh.vertexBuffer = VK_NULL_HANDLE;
            }
            if (submesh.indexBuffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(m_r_context.allocator, submesh.indexBuffer, submesh.indexAlloc);
                submesh.indexBuffer = VK_NULL_HANDLE;
            }
        }
    }

    void VulkanMesh::upload(const MeshData& meshData) {
        log("Uploading mesh with %zu submeshes", meshData.submeshes.size());

        m_submeshBuffers.reserve(meshData.submeshes.size());
        m_submeshTextures.reserve(meshData.submeshes.size());
        m_cpuSubmeshData = meshData.submeshes;

        for (auto& srcSubmesh : m_cpuSubmeshData) {
            SubmeshBuffers buffers{};

            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = srcSubmesh.vertices.size() * sizeof(Vertex);
            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo,
                            &buffers.vertexBuffer, &buffers.vertexAlloc, nullptr);

            void* data;
            vmaMapMemory(m_r_context.allocator, buffers.vertexAlloc, &data);
            memcpy(data, srcSubmesh.vertices.data(), bufferInfo.size);
            vmaUnmapMemory(m_r_context.allocator, buffers.vertexAlloc);

            bufferInfo.size = srcSubmesh.indices.size() * sizeof(uint32_t);
            bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo,
                            &buffers.indexBuffer, &buffers.indexAlloc, nullptr);

            vmaMapMemory(m_r_context.allocator, buffers.indexAlloc, &data);
            memcpy(data, srcSubmesh.indices.data(), bufferInfo.size);
            vmaUnmapMemory(m_r_context.allocator, buffers.indexAlloc);

            buffers.indexCount = static_cast<uint32_t>(srcSubmesh.indices.size());

            m_submeshBuffers.push_back(buffers);
            m_submeshTextures.push_back(srcSubmesh.texturePath);

            for (size_t i = 0; i < srcSubmesh.indices.size(); i += 3) {
                const auto& p0 = srcSubmesh.vertices[srcSubmesh.indices[i+0]].position;
                const auto& p1 = srcSubmesh.vertices[srcSubmesh.indices[i+1]].position;
                const auto& p2 = srcSubmesh.vertices[srcSubmesh.indices[i+2]].position;
                srcSubmesh.triangleCenters.push_back((p0 + p1 + p2) / 3.0f);
            }

            log("Uploaded submesh: %zu vertices, %u indices, texture: '%s'",
                   srcSubmesh.vertices.size(), buffers.indexCount,
                   srcSubmesh.texturePath.c_str());
        }
    }

    void VulkanMesh::extractTransparentTriangles(
        const glm::mat4& modelMatrix,
        const glm::vec3& cameraPos,
        uint32_t modelIndex,
        uint32_t frameIndex,
        std::vector<TransparentTriangle>& outTriangles
    ) {
        glm::mat3 rotation_scale_matrix = glm::mat3(modelMatrix);
        glm::vec3 translation_vector = glm::vec3(modelMatrix[3]);

        for (uint32_t submeshIndex = 0; submeshIndex < m_cpuSubmeshData.size(); ++submeshIndex) {
            const auto& submesh = m_cpuSubmeshData[submeshIndex];
            const size_t indexCount = submesh.indices.size();

            for (uint32_t i = 0; i < indexCount; i += 3) {
                glm::vec3 center_model = submesh.triangleCenters[i/3];

                glm::vec3 center_world = rotation_scale_matrix * center_model + translation_vector;

                glm::vec3 d = center_world - cameraPos;
                float distanceSq = glm::dot(d, d);

                outTriangles.push_back({
                    distanceSq,
                    modelIndex,
                    frameIndex,
                    i,
                    submeshIndex,
                    this
                });
            }
        }
    }

    void VulkanMesh::bindAndDrawBatched(
        VkCommandBuffer cmd,
        VkPipelineLayout pipelineLayout,
        VulkanResources& resources,
        uint32_t frameIndex,
        uint32_t modelIndex,
        uint32_t submeshIndex,
        glm::mat4 modelMatrix
    ) const {
        const auto& buffers = m_submeshBuffers[submeshIndex];
        const auto& textureName = m_submeshTextures[submeshIndex];

        uint32_t textureIndex = resources.getTextureIndex(textureName);

        if (textureIndex >= MAX_TEXTURES) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER,
                       "Invalid texture index %u for '%s' (Max: %u)",
                       textureIndex, textureName.c_str(), MAX_TEXTURES);
            textureIndex = 0;
        }

        uint32_t lightsDynamicOffset = modelIndex * static_cast<uint32_t>(sizeof(SceneLightsUBO));


        std::array<VkDescriptorSet, 2> descriptorSets = {
            resources.getDescriptorSet(frameIndex),
            resources.getTextureDescriptorSet(frameIndex, textureIndex)
        };

        uint32_t dynamicOffsets[1] = { lightsDynamicOffset };

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            descriptorSets.size(),
            descriptorSets.data(),
            1,
            dynamicOffsets
        );

        PushConstants modelPush{};
        const bool textureExists = !textureName.empty() && resources.textureExists(textureName);

        if (textureExists) {
            modelPush.color = glm::vec4(1.0f);
        } else {
            modelPush.color = glm::vec4(1.0f);
            if(!textureName.empty()) {
                log("Missing texture: %s", textureName.c_str());
            }
        }

        modelPush.model = modelMatrix;

        vkCmdPushConstants(
            cmd,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstants),
            &modelPush
        );

        VkBuffer vertexBuffers[] = {buffers.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanMesh::draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
            VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, glm::mat4 modelMatrix, glm::vec4 color) const {

        std::string currentTexture = "";

            uint32_t lightsDynamicOffset = modelIndex * static_cast<uint32_t>(sizeof(SceneLightsUBO));
            uint32_t dynamicOffsets[1] = { lightsDynamicOffset };

            VkDescriptorSet globalSet = resources.getDescriptorSet(frameIndex);
            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,
                1,
                &globalSet,
                1,
                dynamicOffsets
            );

        for (size_t i = 0; i < m_submeshBuffers.size(); i++) {
            const auto& buffers = m_submeshBuffers[i];
            std::string textureName = m_submeshTextures[i];
            uint32_t textureIndex = resources.getTextureIndex(textureName);
            //log("Invalid texture index %u for '%s'", textureIndex, textureName.c_str());

            if (textureIndex >= MAX_TEXTURES) {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER,
                           "Invalid texture index %u for '%s' (Max: %u)",
                           textureIndex, textureName.c_str(), MAX_TEXTURES);
                textureIndex = 0;
            }

            const bool textureExists = !textureName.empty() && resources.textureExists(textureName);

            //log("Texture exists: %s, textureName: %s", textureExists ? "true" : "false", textureName.c_str());

            if (!textureExists) {
                textureName = "default";
            }

            if (currentTexture != textureName) {
                VkImageView textureView = resources.getTextureView(textureName);
                VkDescriptorSet texSet = resources.getTextureDescriptorSet(frameIndex, textureIndex);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                          pipelineLayout, 1, 1, &texSet,
                                          0, nullptr);
                currentTexture = textureName;
            }

            PushConstants modelPush{};
            if (textureExists) {
                if (currentTexture != textureName) {
                    VkImageView textureView = resources.getTextureView(textureName);
                    if(textureView != VK_NULL_HANDLE) {
                        resources.updateTextureDescriptor(frameIndex, textureView, i);
                        currentTexture = textureName;
                    }
                }
                modelPush.color = glm::vec4(1.0f);
            } else {
                modelPush.color = color;
                if(!textureName.empty() && textureName != "default") {
                    log("Missing texture: %s", textureName.c_str());
                }
            }

            modelPush.model = modelMatrix;

            vkCmdPushConstants(
                cmd,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushConstants),
                &modelPush
            );

            VkBuffer vertexBuffers[] = {buffers.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd, buffers.indexCount, 1, 0, 0, 0);
        }
    }
}
