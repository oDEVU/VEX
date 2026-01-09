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

        m_Textures.clear();
        m_ranges.clear();
        m_triangleCenters.clear();

            if (m_Buffer.vertexBuffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(m_r_context.allocator, m_Buffer.vertexBuffer, m_Buffer.vertexAlloc);
                m_Buffer.vertexBuffer = VK_NULL_HANDLE;
            }
            if (m_Buffer.indexBuffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(m_r_context.allocator, m_Buffer.indexBuffer, m_Buffer.indexAlloc);
                m_Buffer.indexBuffer = VK_NULL_HANDLE;
            }
    }

    void VulkanMesh::StreamToGPU(void* dst, const void* src, size_t sizeBytes) {
        auto* dst128 = static_cast<__m128i*>(dst);
        auto* src128 = static_cast<const __m128i*>(src);

        size_t loops = sizeBytes / sizeof(__m128i);
        size_t tail  = sizeBytes % sizeof(__m128i);

        for (size_t i = 0; i < loops; ++i) {
            __m128i data = _mm_loadu_si128(src128 + i);
            _mm_stream_si128(dst128 + i, data);
        }

        if (tail > 0) {
            size_t offset = loops * sizeof(__m128i);
            std::memcpy(
                static_cast<char*>(dst) + offset,
                static_cast<const char*>(src) + offset,
                tail
            );
        }

        _mm_sfence();
    }

    void VulkanMesh::upload(const MeshData& meshData) {
        log("Uploading mesh with %zu submeshes", meshData.submeshes.size());

        if (m_Buffer.vertexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_r_context.allocator, m_Buffer.vertexBuffer, m_Buffer.vertexAlloc);
            m_Buffer.vertexBuffer = VK_NULL_HANDLE;
        }
        if (m_Buffer.indexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_r_context.allocator, m_Buffer.indexBuffer, m_Buffer.indexAlloc);
            m_Buffer.indexBuffer = VK_NULL_HANDLE;
        }

        m_Textures = meshData.texturePaths;
        m_ranges = meshData.submeshes;
        m_cpuMeshData = meshData;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = m_cpuMeshData.vertices.size() * sizeof(Vertex);
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo,
                        &m_Buffer.vertexBuffer, &m_Buffer.vertexAlloc, nullptr);

        void* data;
        vmaMapMemory(m_r_context.allocator, m_Buffer.vertexAlloc, &data);
        StreamToGPU(data, m_cpuMeshData.vertices.data(), bufferInfo.size);
        vmaUnmapMemory(m_r_context.allocator, m_Buffer.vertexAlloc);

        m_Buffer.vertexCount = static_cast<uint32_t>(m_cpuMeshData.vertices.size());

        bufferInfo.size = m_cpuMeshData.indices.size() * sizeof(uint32_t);
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo,
                        &m_Buffer.indexBuffer, &m_Buffer.indexAlloc, nullptr);

        vmaMapMemory(m_r_context.allocator, m_Buffer.indexAlloc, &data);
        StreamToGPU(data, m_cpuMeshData.indices.data(), bufferInfo.size);
        vmaUnmapMemory(m_r_context.allocator, m_Buffer.indexAlloc);

        m_Buffer.indexCount = static_cast<uint32_t>(m_cpuMeshData.indices.size());

        m_triangleCenters.clear();
        m_triangleSubmeshIndices.clear();

        size_t totalTriangles = m_cpuMeshData.indices.size() / 3;
        m_triangleCenters.reserve(totalTriangles);
        m_triangleSubmeshIndices.reserve(totalTriangles);

        for (size_t i = 0; i < m_cpuMeshData.indices.size(); i += 3) {
            // Safety check
            if (i + 2 >= m_cpuMeshData.indices.size()) break;

            const auto& p0 = m_cpuMeshData.vertices[m_cpuMeshData.indices[i+0]].position;
            const auto& p1 = m_cpuMeshData.vertices[m_cpuMeshData.indices[i+1]].position;
            const auto& p2 = m_cpuMeshData.vertices[m_cpuMeshData.indices[i+2]].position;
            m_triangleCenters.push_back((p0 + p1 + p2) / 3.0f);
        }

        for (size_t subIdx = 0; subIdx < m_ranges.size(); subIdx++) {
            const auto& range = m_ranges[subIdx];
            size_t triCount = range.indexCount / 3;
            for (size_t k = 0; k < triCount; k++) {
                m_triangleSubmeshIndices.push_back(static_cast<uint8_t>(subIdx));
            }
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

        size_t triCount = m_triangleCenters.size();
        for (size_t i = 0; i < triCount; ++i) {
            glm::vec3 center_world = rotation_scale_matrix * m_triangleCenters[i] + translation_vector;

            glm::vec3 d = center_world - cameraPos;
            float distanceSq = glm::dot(d, d);

            outTriangles.push_back({
                distanceSq,
                modelIndex,
                frameIndex,
                static_cast<uint32_t>(i * 3),
                static_cast<uint32_t>(m_triangleSubmeshIndices[i]),
                this,
                entity
            });
        }
    }

    void VulkanMesh::draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex,
                glm::mat4 modelMatrix, const MeshComponent& mc) const {

            uint32_t lightsOffset = modelIndex * sizeof(SceneLightsUBO);
            VkDescriptorSet frameSet = resources.getDescriptorSet(frameIndex);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                    0, 1, &frameSet, 1, &lightsOffset);

            VkBuffer vertexBuffers[] = {m_Buffer.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, m_Buffer.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            if (m_r_context.supportsBindlessTextures) [[likely]] {
                 PushConstants pc{};
                 pc.color = mc.color;
                 pc.model = modelMatrix;

                 size_t count = std::min(m_Textures.size(), (size_t)12);
                 for(size_t i = 0; i < count; i++) {
                     std::string texName = m_Textures[i];
                     if(mc.textureOverrides.contains(i)) texName = GetAssetPath(mc.textureOverrides.at(i));
                     pc.textureIDs[i] = resources.getTextureIndex(texName);
                 }

                 vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                    0, sizeof(PushConstants), &pc);

                 vkCmdDrawIndexed(cmd, m_Buffer.indexCount, 1, 0, 0, 0);
            } else [[unlikely]] {
                 std::string currentBoundTex = "";

                 for (size_t i = 0; i < m_ranges.size(); i++) {
                     const auto& range = m_ranges[i];

                     std::string texName;
                     if (mc.textureOverrides.contains(i)) {
                         texName = GetAssetPath(mc.textureOverrides.at(i));
                     } else {
                         texName = m_Textures[range.textureIndex];
                     }

                     if (texName != currentBoundTex) {
                         uint32_t idx = resources.getTextureIndex(texName);
                         VkDescriptorSet texSet = resources.getTextureDescriptorSet(frameIndex, idx);
                         vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                                 1, 1, &texSet, 0, nullptr);
                         currentBoundTex = texName;
                     }

                     PushConstants pc{};
                     pc.color = mc.color;
                     pc.model = modelMatrix;

                     vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                        0, sizeof(PushConstants), &pc);

                     vkCmdDrawIndexed(cmd, range.indexCount, 1, range.firstIndex, 0, 0);
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

            if (modelChanged) {
                uint32_t lightsOffset = modelIndex * sizeof(SceneLightsUBO);
                VkDescriptorSet frameSet = resources.getDescriptorSet(frameIndex);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                        0, 1, &frameSet, 1, &lightsOffset);

                VkBuffer vertexBuffers[] = {m_Buffer.vertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(cmd, m_Buffer.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }

            if (m_r_context.supportsBindlessTextures) [[likely]] {
                 if (modelChanged) {
                     PushConstants pc{};
                     pc.color = mc.color;
                     pc.model = modelMatrix;

                     size_t count = std::min(m_Textures.size(), (size_t)12);
                     for(size_t i = 0; i < count; i++) {
                         std::string texName = m_Textures[i];
                         if(mc.textureOverrides.contains(i)) texName = GetAssetPath(mc.textureOverrides.at(i));
                         pc.textureIDs[i] = resources.getTextureIndex(texName);
                     }
                     vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                        0, sizeof(PushConstants), &pc);
                 }
            } else [[unlikely]] {
                 if (submeshIndex < m_ranges.size()) {
                     const auto& range = m_ranges[submeshIndex];

                     std::string texName;
                     if (mc.textureOverrides.contains(submeshIndex)) {
                         texName = GetAssetPath(mc.textureOverrides.at(submeshIndex));
                     } else {
                         texName = m_Textures[range.textureIndex];
                     }

                     uint32_t idx = resources.getTextureIndex(texName);
                     VkDescriptorSet texSet = resources.getTextureDescriptorSet(frameIndex, idx);
                     vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                             1, 1, &texSet, 0, nullptr);

                     if (modelChanged) {
                        PushConstants pc{};
                        pc.color = mc.color;
                        pc.model = modelMatrix;
                        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                           0, sizeof(PushConstants), &pc);
                     }
                 }
            }
        }
}
