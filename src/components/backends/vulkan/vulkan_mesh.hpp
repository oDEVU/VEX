#pragma once
#include "context.hpp"
#include "../../mesh.hpp" // Add this include

namespace vex {
    class VulkanMesh {
    public:
        VulkanMesh(VulkanContext& context);
        ~VulkanMesh();

        void upload(const MeshData& meshData); // Fixed parameter type
        void draw(VkCommandBuffer cmd) const;

    private:
        VulkanContext& ctx_;
        VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
        VkBuffer indexBuffer_ = VK_NULL_HANDLE;
        VmaAllocation vertexAlloc_ = VK_NULL_HANDLE;
        VmaAllocation indexAlloc_ = VK_NULL_HANDLE;
        uint32_t indexCount_ = 0;
    };
}
