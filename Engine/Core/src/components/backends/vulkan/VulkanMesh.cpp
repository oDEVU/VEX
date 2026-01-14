#include "VulkanMesh.hpp"
#include "components/Mesh.hpp"
#include "components/pathUtils.hpp"
#include "glm/common.hpp"
#include "glm/fwd.hpp"

#include <SDL3/SDL.h>

// debug
#ifdef _WIN32
// pass
#else
#include <X11/X.h>
#endif
#include <cstdint>
#include <immintrin.h>
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

    void VulkanMesh::StreamToGPU(void* dst, const void* src, size_t sizeBytes) {
        char* dstPtr = static_cast<char*>(dst);
        const char* srcPtr = static_cast<const char*>(src);

        size_t dstAddr = reinterpret_cast<size_t>(dstPtr);
        size_t misalignment = dstAddr & 15;
        size_t head = (misalignment == 0) ? 0 : (16 - misalignment);

        if (head > sizeBytes) head = sizeBytes;

        if (head > 0) {
            std::memcpy(dstPtr, srcPtr, head);
            dstPtr += head;
            srcPtr += head;
            sizeBytes -= head;
        }

        size_t vecSize = sizeof(__m128i);
        size_t loops = sizeBytes / vecSize;

        auto* dst128 = reinterpret_cast<__m128i*>(dstPtr);
        auto* src128 = reinterpret_cast<const __m128i*>(srcPtr);

        for (size_t i = 0; i < loops; ++i) {
            __m128i data = _mm_loadu_si128(src128 + i);
            _mm_stream_si128(dst128 + i, data);
        }

        size_t bytesHandled = loops * vecSize;
        size_t tail = sizeBytes - bytesHandled;

        if (tail > 0) {
            std::memcpy(dstPtr + bytesHandled, srcPtr + bytesHandled, tail);
        }

        _mm_sfence();
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
            StreamToGPU(data, srcSubmesh.vertices.data(), bufferInfo.size);
            vmaUnmapMemory(m_r_context.allocator, buffers.vertexAlloc);

            bufferInfo.size = srcSubmesh.indices.size() * sizeof(uint32_t);
            bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo,
                            &buffers.indexBuffer, &buffers.indexAlloc, nullptr);

            vmaMapMemory(m_r_context.allocator, buffers.indexAlloc, &data);
            StreamToGPU(data, srcSubmesh.indices.data(), bufferInfo.size);
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
        entt::entity entity,
        std::vector<TransparentTriangle>& outTriangles
    ) {
        glm::mat3 rotation_scale_matrix = glm::mat3(modelMatrix);
        glm::vec3 translation_vector = glm::vec3(modelMatrix[3]);

        for (uint32_t submeshIndex = 0; submeshIndex < m_cpuSubmeshData.size(); ++submeshIndex) {
            const auto& submesh = m_cpuSubmeshData[submeshIndex];
            const size_t indexCount = submesh.indices.size();
            const glm::vec3* centers = submesh.triangleCenters.data();

            for (uint32_t i = 0; i < indexCount; i += 3) {
                glm::vec3 center_world = rotation_scale_matrix * centers[i/3] + translation_vector;

                glm::vec3 d = center_world - cameraPos;
                float distanceSq = glm::dot(d, d);

                outTriangles.push_back({
                    distanceSq,
                    modelIndex,
                    frameIndex,
                    i,
                    submeshIndex,
                    this,
                    entity
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
        glm::mat4 modelMatrix,
        bool modelChanged,
        bool submeshChanged,
        const MeshComponent& mc
    ) const {
        const auto& buffers = m_submeshBuffers[submeshIndex];
        const auto& textureName = m_submeshTextures[submeshIndex];

        if(modelChanged){
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
        }

        uint32_t textureIndex = resources.getTextureIndex(textureName);
        if (textureIndex >= MAX_TEXTURES) {
            textureIndex = 0;
        }

        if (m_r_context.supportsBindlessTextures) {

        }
        else {
            if(submeshChanged || modelChanged){
                VkDescriptorSet texSet = resources.getTextureDescriptorSet(frameIndex, textureIndex);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineLayout, 1, 1, &texSet,
                                        0, nullptr);
            }
        }

        bool needPush = modelChanged;
        if (m_r_context.supportsBindlessTextures && submeshChanged) {
            needPush = true;
        }

        if(needPush){
            PushConstants modelPush{};
            modelPush.model = modelMatrix;
            modelPush.color = mc.color;
            modelPush.textureID = textureIndex;

            if (!resources.textureExists(textureName) && !textureName.empty()) {
                log(LogLevel::WARNING, "Missing texture...");
            }

            vkCmdPushConstants(
                cmd,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushConstants),
                &modelPush
            );
        }

        VkBuffer vertexBuffers[] = {buffers.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanMesh::draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
            VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, glm::mat4 modelMatrix, const MeshComponent& mc) const {

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
            std::string textureName = "";
            uint32_t textureIndex = 0;
            if(mc.textureOverrides.contains(i)){
                textureName = GetAssetPath(mc.textureOverrides.at(i));
                textureIndex = resources.getTextureIndex(textureName);

                if(textureIndex == 0){
                    textureIndex = resources.getTextureIndex(m_submeshTextures[i]);
                }
            }else{
                textureName = m_submeshTextures[i];
                textureIndex = resources.getTextureIndex(m_submeshTextures[i]);
            }

            //log("Invalid texture index %u for '%s'", textureIndex, textureName.c_str());

            if (textureIndex >= MAX_TEXTURES) {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER,
                           "Invalid texture index %u for '%s' (Max: %u)",
                           textureIndex, textureName.c_str(), MAX_TEXTURES);
                textureIndex = 0;
            }

            const bool textureExists = !textureName.empty() && resources.textureExists(textureName);

            //log("Texture exists: %s, textureName: %s, textureIndex: %d", textureExists ? "true" : "false", textureName.c_str(), textureIndex);

            if (!textureExists) {
                textureName = "default";
            }

            PushConstants modelPush{};

            if (m_r_context.supportsBindlessTextures) {
                    PushConstants modelPush{};
                    modelPush.color = mc.color;
                    modelPush.model = modelMatrix;
                    modelPush.textureID = textureIndex;

                    vkCmdPushConstants(
                        cmd,
                        pipelineLayout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        sizeof(PushConstants),
                        &modelPush
                    );
                }
                else {
                    if (currentTexture != textureName) {
                         VkDescriptorSet texSet = resources.getTextureDescriptorSet(frameIndex, textureIndex);
                         vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &texSet, 0, nullptr);
                         currentTexture = textureName;
                    }

                    PushConstants modelPush{};
                    modelPush.color = mc.color;
                    modelPush.model = modelMatrix;

                    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &modelPush);
                }

            VkBuffer vertexBuffers[] = {buffers.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd, buffers.indexCount, 1, 0, 0, 0);
        }
    }
}
