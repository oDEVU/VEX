#include "vulkan_mesh.hpp"
#include "../../mesh.hpp" // Add this include

namespace vex {
    VulkanMesh::VulkanMesh(VulkanContext& context) : ctx_(context) {}

    VulkanMesh::~VulkanMesh() {
        if (vertexBuffer_) {
            vmaDestroyBuffer(ctx_.allocator, vertexBuffer_, vertexAlloc_);
            vertexBuffer_ = VK_NULL_HANDLE;
        }
        if (indexBuffer_) {
            vmaDestroyBuffer(ctx_.allocator, indexBuffer_, indexAlloc_);
            indexBuffer_ = VK_NULL_HANDLE;
        }
    }

    void VulkanMesh::upload(const MeshData& meshData) {
        // Cleanup old buffers if they exist
        if (vertexBuffer_) vmaDestroyBuffer(ctx_.allocator, vertexBuffer_, vertexAlloc_);
        if (indexBuffer_) vmaDestroyBuffer(ctx_.allocator, indexBuffer_, indexAlloc_);

        // Vertex buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = meshData.vertices.size() * sizeof(Vertex);
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        if (vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo,
                          &vertexBuffer_, &vertexAlloc_, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vertex buffer");
        }

        // Upload vertices
        void* data;
        vmaMapMemory(ctx_.allocator, vertexAlloc_, &data);
        memcpy(data, meshData.vertices.data(), bufferInfo.size);
        vmaUnmapMemory(ctx_.allocator, vertexAlloc_);

        // Index buffer
        bufferInfo.size = meshData.indices.size() * sizeof(uint32_t);
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        indexCount_ = static_cast<uint32_t>(meshData.indices.size());

        if (vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo,
                          &indexBuffer_, &indexAlloc_, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create index buffer");
        }

        // Upload indices
        vmaMapMemory(ctx_.allocator, indexAlloc_, &data);
        memcpy(data, meshData.indices.data(), bufferInfo.size);
        vmaUnmapMemory(ctx_.allocator, indexAlloc_);
    }

    void VulkanMesh::draw(VkCommandBuffer cmd) const {
        if (indexCount_ == 0) return;

        VkBuffer vertexBuffers[] = {vertexBuffer_};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, indexCount_, 1, 0, 0, 0);
    }
}
