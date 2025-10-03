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

        for (const auto& srcSubmesh : meshData.submeshes) {
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

            log("Uploaded submesh: %zu vertices, %u indices, texture: '%s'",
                   srcSubmesh.vertices.size(), buffers.indexCount,
                   srcSubmesh.texturePath.c_str());
        }
    }

    void VulkanMesh::draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                         VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, float currentTime, glm::uvec2 currentRenderResolution) const {
        if(!m_debugDraw) log("Drawing mesh with %zu submeshes", m_submeshBuffers.size());

        std::string currentTexture = "";
        for (size_t i = 0; i < m_submeshBuffers.size(); i++) {
            const auto& buffers = m_submeshBuffers[i];
            const auto& textureName = m_submeshTextures[i];
            uint32_t textureIndex = resources.getTextureIndex(textureName);
            //log("Invalid texture index %u for '%s'", textureIndex, textureName.c_str());

            if (textureIndex >= m_r_context.MAX_TEXTURES) {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER,
                           "Invalid texture index %u for '%s' (Max: %u)",
                           textureIndex, textureName.c_str(), m_r_context.MAX_TEXTURES);
                textureIndex = 0;
            }

            uint32_t dynamicOffset = modelIndex * static_cast<uint32_t>(sizeof(ModelUBO));

            std::array<VkDescriptorSet, 2> descriptorSets = {
                resources.getDescriptorSet(frameIndex),
                resources.getTextureDescriptorSet(frameIndex, textureIndex)
            };

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,
                descriptorSets.size(),
                descriptorSets.data(),
                1,
                &dynamicOffset
            );

            const bool textureExists = !textureName.empty() && resources.textureExists(textureName);
            if(!m_debugDraw) log("Submesh %zu texture: '%s' (exists: %s)",
                   i,
                   textureName.c_str(),
                   textureExists ? "true" : "false");

            if (textureExists) {
                if (currentTexture != textureName) {
                    VkImageView textureView = resources.getTextureView(textureName);
                    if (textureView != VK_NULL_HANDLE) {
                        VkDescriptorSet texSet = resources.getTextureDescriptorSet(frameIndex, textureIndex);
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                              pipelineLayout, 1, 1, &texSet,
                                              0, nullptr);
                        currentTexture = textureName;
                    }
                }
            }

            PushConstants modelPush{};
            if (textureExists) {
                if (currentTexture != textureName) {
                    VkImageView textureView = resources.getTextureView(textureName);
                    if(textureView != VK_NULL_HANDLE) {
                        resources.updateTextureDescriptor(frameIndex, textureView, i);
                        currentTexture = textureName;
                        if(!m_debugDraw) log("Bound texture: %s", textureName.c_str());
                    }
                }
                modelPush.color = glm::vec4(1.0f);
            } else {
                modelPush.color = glm::vec4(1.0f);
                if(!textureName.empty()) {
                    if(!m_debugDraw) SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                              "Missing texture: %s", textureName.c_str());
                }
            }

            modelPush.snapResolution = 1.f;
            modelPush.jitterIntensity = 0.5f;

            if(m_r_context.m_enviroment.vertexSnapping){
                modelPush.enablePS1Effects |= PS1Effects::VERTEX_SNAPPING;
            }

            if(m_r_context.m_enviroment.passiveVertexJitter){
                modelPush.enablePS1Effects |= PS1Effects::VERTEX_JITTER;
            }

            if(m_r_context.m_enviroment.affineWarping){
                modelPush.enablePS1Effects |= PS1Effects::AFFINE_WARPING;
            }

            if(m_r_context.m_enviroment.colorQuantization){
                modelPush.enablePS1Effects |= PS1Effects::COLOR_QUANTIZATION;
            }

            if(m_r_context.m_enviroment.ntfsArtifacts){
                modelPush.enablePS1Effects |= PS1Effects::NTSC_ARTIFACTS;
            }

            if(m_r_context.m_enviroment.gourardShading){
                modelPush.enablePS1Effects |= PS1Effects::GOURAUD_SHADING;
            }

            modelPush.renderResolution = currentRenderResolution;
            modelPush.windowResolution = {m_r_context.swapchainExtent.width, m_r_context.swapchainExtent.height};
            modelPush.time = currentTime;
            modelPush.upscaleRatio = m_r_context.swapchainExtent.height / static_cast<float>(currentRenderResolution.y);

            modelPush.ambientLight = glm::vec4(m_r_context.m_enviroment.ambientLight,1.0f);
            modelPush.ambientLightStrength = m_r_context.m_enviroment.ambientLightStrength;
            modelPush.sunLight = glm::vec4(m_r_context.m_enviroment.sunLight,1.0f);
            modelPush.sunDirection = glm::vec4(m_r_context.m_enviroment.sunDirection,1.0f);

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
