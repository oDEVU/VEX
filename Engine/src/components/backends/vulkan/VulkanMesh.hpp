#pragma once
#include "context.hpp"
#include "components/Mesh.hpp"
#include "components/Model.hpp"
#include "components/errorUtils.hpp"
#include "Resources.hpp"

namespace vex {
    class VulkanMesh {
    public:
        VulkanMesh(VulkanContext& context);
        ~VulkanMesh();

        void upload(const MeshData& meshData);
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

        VulkanContext& m_r_context;
        std::vector<SubmeshBuffers> m_submeshBuffers;
        std::vector<std::string> m_submeshTextures;
        bool m_debugDraw = true; // set to true when not debuging lmao
    };
}
