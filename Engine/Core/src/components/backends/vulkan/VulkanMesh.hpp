/**
 *  @file   VulkanMesh.hpp
 *  @brief  This file defines VulkanMesh class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "context.hpp"
#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"
#include "Resources.hpp"
#include <sys/types.h>

namespace vex {
    /// @brief Class defining mesh object with vulkan specific data and methods.
    class VulkanMesh {
    public:
        /// @brief Constructor for VulkanMesh class.
        /// @param VulkanContext& context - Reference to the VulkanContext object.
        VulkanMesh(VulkanContext& context);
        ~VulkanMesh();

        /// @brief Uploads mesh data to the GPU.
        /// @param const MeshData& meshData - Mesh data to be uploaded.
        void upload(const MeshData& meshData);

        /// @brief Draws the mesh to the screen.
        /// @param VkCommandBuffer cmd - Command buffer to draw the mesh.
        /// @param VkPipelineLayout pipelineLayout - Pipeline layout for the mesh.
        /// @param VulkanResources& resources - Vulkan resources for the mesh.
        /// @param uint32_t frameIndex - Index of the current frame.
        /// @param uint32_t modelIndex - Index of the current model.
        /// @param float currentTime - Current time.
        /// @param glm::uvec2 currentRenderResolution - Current render resolution.
        /// @details It is called automatically in Renderer for every mesh component having a VulkanMesh.
        void draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, float currentTime, glm::uvec2 currentRenderResolution) const;

        /// @brief Helper function to get number of mesh components using this VulkanMesh instance, needed for mesh manager to know when to unload VulkanMesh.
        /// @return u_int32_t
        u_int32_t getNumOfInstances() const { return numOfInstances; }

        /// @brief increments number of mesh components using this VulkanMesh instance.
        void addInstance() { numOfInstances++; }

        /// @brief decrements number of mesh components using this VulkanMesh instance.
        void removeInstance() { numOfInstances--; }

    private:
        /// @brief struct used to hold buffers and allocations for each submesh in single object.
        struct SubmeshBuffers {
            VkBuffer vertexBuffer;
            VmaAllocation vertexAlloc;
            VkBuffer indexBuffer;
            VmaAllocation indexAlloc;
            uint32_t indexCount;
        };

        VulkanContext& m_r_context;
        std::vector<SubmeshBuffers> m_submeshBuffers;
        std::vector<std::string> m_submeshTextures;
        u_int32_t numOfInstances = 0;
    };
}
