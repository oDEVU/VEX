#pragma once
#include "context.hpp"
#include "components/mesh.hpp"
#include "components/model.hpp"
#include "resources.hpp"

namespace vex {
    class VulkanMesh {
    public:
        VulkanMesh(VulkanContext& context);
        ~VulkanMesh();

        void upload(const MeshData& meshData);
        // Add pipeline layout parameter for push constants
        void draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, float currentTime, glm::uvec2 currentRenderResolution) const;

    private:
        struct SubmeshBuffers {
            VkBuffer vertexBuffer;
            VmaAllocation vertexAlloc;
            VkBuffer indexBuffer;
            VmaAllocation indexAlloc;
            uint32_t indexCount;
        };

        void disableDebug();

        VulkanContext& ctx_;
        std::vector<SubmeshBuffers> submeshBuffers_;
        std::vector<std::string> submeshTextures_;  // Parallel to submeshBuffers_
        bool debugDraw = true; // set to true when not debuging lmao
    };
}
