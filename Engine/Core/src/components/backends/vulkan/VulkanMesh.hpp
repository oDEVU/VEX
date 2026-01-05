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

#include "components/GameComponents/BasicComponents.hpp"

#include <sys/types.h>

namespace vex {
    struct TransparentTriangle;

    /// @brief Info needed for drawing indexed indirect commands.
    struct DrawIndexedIndirectCommand;

    /// @brief Class defining mesh object with vulkan specific data and methods.
    class VulkanMesh {
    public:
        /// @brief Constructor for VulkanMesh class.
        /// @param VulkanContext& context - Reference to the VulkanContext object.
        VulkanMesh(VulkanContext& context);
        ~VulkanMesh();

        /// @brief Uploads mesh data to the GPU.
        /// @details Creates VMA buffers (`VK_BUFFER_USAGE_VERTEX_BUFFER_BIT` / `INDEX`) and uploads data from `MeshData`. Stores CPU copies of triangle centers for transparency sorting.
        /// @param const MeshData& meshData - The source mesh data.
        void upload(const MeshData& meshData);

        // @brief Function to extract transparent triangles
        // @details Transforms triangle centers to world space, calculates distance to camera, and populates `outTriangles` with render data.
        // @param const glm::mat4& modelMatrix - Model matrix for the mesh.
        // @param const glm::vec3& cameraPos - Position of the camera.
        // @param uint32_t modelIndex - Index of the current model.
        // @param uint32_t frameIndex - Index of the current frame.
        // @param entt::entity entity - Entity associated with the mesh.
        // @param std::vector<TransparentTriangle>& outTriangles - Vector to store extracted transparent triangles.
        void extractTransparentTriangles(
            const glm::mat4& modelMatrix,
            const glm::vec3& cameraPos,
            uint32_t modelIndex,
            uint32_t frameIndex,
            entt::entity entity,
            std::vector<TransparentTriangle>& outTriangles
        );

        /// @brief Draws the mesh to the screen.
        /// @details Binds buffers, descriptors, and issues draw calls for each submesh. Resolves texture overrides from `MeshComponent`.
        /// @param VkCommandBuffer cmd - Command buffer to draw the mesh.
        /// @param VkPipelineLayout pipelineLayout - Pipeline layout for the mesh.
        /// @param VulkanResources& resources - Vulkan resources for the mesh.
        /// @param uint32_t frameIndex - Index of the current frame.
        /// @param uint32_t modelIndex - Index of the current model.
        /// @param float currentTime - Current time.
        /// @param glm::uvec2 currentRenderResolution - Current render resolution.
        /// @details It is called automatically in Renderer for every mesh component having a VulkanMesh.
        void draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                VulkanResources& resources, uint32_t frameIndex, uint32_t modelIndex, glm::mat4 modelMatrix, const MeshComponent& mc) const;

        /// @brief Batch-optimized draw call for sorted transparent triangles.
        /// @details Only re-binds descriptors/buffers if `modelChanged` or `submeshChanged` is true. Used in conjunction with `issueMultiDrawIndexed`.
        /// @param VkCommandBuffer cmd - Command buffer.
        /// @param VkPipelineLayout pipelineLayout - Pipeline layout.
        /// @param VulkanResources& resources - Resource manager.
        /// @param uint32_t frameIndex - Frame index.
        /// @param uint32_t modelIndex - Model index.
        /// @param uint32_t submeshIndex - Submesh index.
        /// @param glm::mat4 modelMatrix - Transform matrix.
        /// @param bool modelChanged - Flag indicating if model-level data (matrix/lights) needs rebinding.
        /// @param bool submeshChanged - Flag indicating if submesh-level data (buffers/textures) needs rebinding.
        /// @param const MeshComponent& mc - Component data.
        void bindAndDrawBatched(
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
        entt::entity entity;

        //glm::mat4 modelMatrix;
    };
}
