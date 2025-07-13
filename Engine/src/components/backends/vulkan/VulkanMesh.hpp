#pragma once
#include "context.hpp"
#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"
#include "Resources.hpp"
#include <sys/types.h>

namespace vex {
    class VulkanMesh {
    public:
        VulkanMesh(VulkanContext& context);
        ~VulkanMesh();

        void upload(const MeshData& meshData);
        void draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, float currentTime, glm::uvec2 currentRenderResolution) const;

        u_int32_t getNumOfInstances() const { return numOfInstances; }
        void addInstance() { numOfInstances++; }
        void removeInstance() { numOfInstances--; }

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
        u_int32_t numOfInstances = 0;
        bool m_debugDraw = true; // set to true when not debuging lmao
    };
}
