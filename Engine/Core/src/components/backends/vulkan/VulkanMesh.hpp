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
    struct TransparentTriangle;
    struct DrawIndexedIndirectCommand;

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

        // @brief Function to extract transparent triangles
        void extractTransparentTriangles(
            const glm::mat4& modelMatrix,
            const glm::vec3& cameraPos,
            uint32_t modelIndex,
            uint32_t frameIndex,
            std::vector<TransparentTriangle>& outTriangles
        );

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
                VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, float currentTime, glm::uvec2 currentRenderResolution, std::vector<Light> lights) const;

        // @brief Function to draw a single triangle
            void drawTriangle(
                VkCommandBuffer cmd,
                VkPipelineLayout pipelineLayout,
                VulkanResources& resources,
                uint32_t frameIndex,
                uint32_t modelIndex,
                uint32_t submeshIndex,
                uint32_t firstIndex,
                float currentTime,
                glm::uvec2 currentRenderResolution
            ) const;

            // @brief This function contains all the setup logic (binding buffers, descriptor sets, and pushing constants) that only needs to be performed when the mesh or submesh (and thus the buffers/texture) changes.
            void bindAndDrawBatched(
                VkCommandBuffer cmd,
                VkPipelineLayout pipelineLayout,
                VulkanResources& resources,
                uint32_t frameIndex,
                uint32_t modelIndex,
                uint32_t submeshIndex,
                float currentTime,
                glm::uvec2 currentRenderResolution
            ) const;

        /// @brief Helper function to get number of mesh components using this VulkanMesh instance, needed for mesh manager to know when to unload VulkanMesh.
        /// @return int
        int getNumOfInstances() const { return numOfInstances; }

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
        int numOfInstances = 0;

        std::vector<Submesh> m_cpuSubmeshData;
    };

/// @brief struct used to held transparent triangles data for sorting and special rendering
struct TransparentTriangle {
        // @brief Distance from the camera to the triangle's center (for sorting)
        float distanceToCamera;

        // @brief Data needed to render the triangle after sorting
        uint32_t modelIndex;
        uint32_t frameIndex;
        uint32_t firstIndex;
        uint32_t submeshIndex;
        VulkanMesh* mesh;
    };
}
