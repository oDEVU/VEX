#include "vulkan_mesh.hpp"
#include "../../mesh.hpp" // Add this include


// debug
#include <cstdint>
#include <cstring>
#include <iostream>

#include <SDL3/SDL.h>

namespace vex {
    VulkanMesh::VulkanMesh(VulkanContext& context) : ctx_(context) {
        SDL_Log("VulkanMesh created");
    }

    VulkanMesh::~VulkanMesh() {
        SDL_Log("Destroying VulkanMesh");
        for (auto& submesh : submeshBuffers_) {
            vmaDestroyBuffer(ctx_.allocator, submesh.vertexBuffer, submesh.vertexAlloc);
            vmaDestroyBuffer(ctx_.allocator, submesh.indexBuffer, submesh.indexAlloc);
        }
    }

    void VulkanMesh::upload(const MeshData& meshData) {
        SDL_Log("Uploading mesh with %zu submeshes", meshData.submeshes.size());

        submeshBuffers_.reserve(meshData.submeshes.size());
        submeshTextures_.reserve(meshData.submeshes.size());

        for (const auto& srcSubmesh : meshData.submeshes) {
            SubmeshBuffers buffers{};

            // Create vertex buffer
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = srcSubmesh.vertices.size() * sizeof(Vertex);
            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo,
                            &buffers.vertexBuffer, &buffers.vertexAlloc, nullptr);

            // Upload vertices
            void* data;
            vmaMapMemory(ctx_.allocator, buffers.vertexAlloc, &data);
            memcpy(data, srcSubmesh.vertices.data(), bufferInfo.size);
            vmaUnmapMemory(ctx_.allocator, buffers.vertexAlloc);

            // Create index buffer
            bufferInfo.size = srcSubmesh.indices.size() * sizeof(uint32_t);
            bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo,
                            &buffers.indexBuffer, &buffers.indexAlloc, nullptr);

            // Upload indices
            vmaMapMemory(ctx_.allocator, buffers.indexAlloc, &data);
            memcpy(data, srcSubmesh.indices.data(), bufferInfo.size);
            vmaUnmapMemory(ctx_.allocator, buffers.indexAlloc);

            buffers.indexCount = static_cast<uint32_t>(srcSubmesh.indices.size());

            submeshBuffers_.push_back(buffers);
            submeshTextures_.push_back(srcSubmesh.texturePath);

            SDL_Log("Uploaded submesh: %zu vertices, %u indices, texture: '%s'",
                   srcSubmesh.vertices.size(), buffers.indexCount,
                   srcSubmesh.texturePath.c_str());
        }
    }

    void VulkanMesh::draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                         VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, float currentTime) const {
        if(!debugDraw) SDL_Log("Drawing mesh with %zu submeshes", submeshBuffers_.size());

        std::string currentTexture = "";
        for (size_t i = 0; i < submeshBuffers_.size(); i++) {
            const auto& buffers = submeshBuffers_[i];
            const auto& textureName = submeshTextures_[i];
            uint32_t textureIndex = resources.getTextureIndex(textureName);


            if (textureIndex >= ctx_.MAX_TEXTURES) {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER,
                           "Invalid texture index %u for '%s' (Max: %u)",
                           textureIndex, textureName.c_str(), ctx_.MAX_TEXTURES);
                textureIndex = 0; // Fallback to default
            }

            // Calculate dynamic offset for this model
            uint32_t dynamicOffset = modelIndex * static_cast<uint32_t>(sizeof(ModelUBO));

            // Bind descriptor sets with dynamic offset
            std::array<VkDescriptorSet, 2> descriptorSets = {
                resources.getDescriptorSet(frameIndex),
                resources.getTextureDescriptorSet(frameIndex, textureIndex)
            };

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,  // First set index
                descriptorSets.size(),
                descriptorSets.data(),
                1,  // Number of dynamic offsets
                &dynamicOffset  // Pointer to offsets array
            );

            // Texture validation and logging
            const bool textureExists = !textureName.empty() && resources.textureExists(textureName);
            if(!debugDraw) SDL_Log("Submesh %zu texture: '%s' (exists: %s)",
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

            PushConstants push{};
            if (textureExists) {
                // Only update descriptor if texture actually exists
                if (currentTexture != textureName) {
                    VkImageView textureView = resources.getTextureView(textureName);
                    if(textureView != VK_NULL_HANDLE) {
                        resources.updateTextureDescriptor(frameIndex, textureView, i);
                        currentTexture = textureName;
                        if(!debugDraw) SDL_Log("Bound texture: %s", textureName.c_str());
                    }
                }
                push.color = glm::vec4(1.0f); // Neutral multiplier
            } else {
                push.color = glm::vec4(1.0f); // Debug color (later replace with vertex color if possible)
                if(!textureName.empty()) {
                    if(!debugDraw) SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                              "Missing texture: %s", textureName.c_str());
                }
            }

            glm::mat3 warpMatrix = glm::mat3(1.0f);
            push.setAffineTransform(warpMatrix);

            // Vertex snap parameters
            push.time = currentTime;
            push.snapResolution = 1.f;
            push.screenSize = {ctx_.swapchainExtent.width, ctx_.swapchainExtent.height};
            push.jitterIntensity = 0.5f;
            push.enablePS1Effects =
                PS1Effects::VERTEX_SNAPPING |
                PS1Effects::AFFINE_WARPING |
                PS1Effects::COLOR_QUANTIZATION |
                PS1Effects::VERTEX_JITTER |
                PS1Effects::NTSC_ARTIFACTS;

            vkCmdPushConstants(cmd, pipelineLayout,
                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                              0, sizeof(PushConstants), &push);

            // Buffer binding
            VkBuffer vertexBuffers[] = {buffers.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd, buffers.indexCount, 1, 0, 0, 0);
        }
    }
}
