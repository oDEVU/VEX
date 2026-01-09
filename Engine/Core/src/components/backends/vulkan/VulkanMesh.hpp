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
        /// @param const MeshData& meshData - Mesh data to be uploaded.
        void upload(const MeshData& meshData);

        // @brief Function to extract transparent triangles
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

<<<<<<< Updated upstream
            // @brief This function contains all the setup logic (binding buffers, descriptor sets, and pushing constants) that only needs to be performed when the mesh or submesh (and thus the buffers/texture) changes.
            // @param const glm::mat4& modelMatrix - Model matrix for the mesh.
            // @param const glm::vec3& cameraPos - Position of the camera.
            // @param uint32_t modelIndex - Index of the current model.
            // @param uint32_t frameIndex - Index of the current frame.
            // @param entt::entity entity - Entity associated with the mesh.
            // @param std::vector<TransparentTriangle>& outTriangles - Vector to store extracted transparent triangles.
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
=======
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
>>>>>>> Stashed changes

        /// @brief Helper function to get number of mesh components using this VulkanMesh instance, needed for mesh manager to know when to unload VulkanMesh.
        /// @return int
        int getNumOfInstances() const { return numOfInstances; }

        /// @brief increments number of mesh components using this VulkanMesh instance.
        void addInstance() { numOfInstances++; }

        /// @brief decrements number of mesh components using this VulkanMesh instance.
        void removeInstance() { numOfInstances--; }

    private:
        /// @brief struct used to hold buffers and allocations for each submesh in single object.
        struct Buffer {
            VkBuffer vertexBuffer = VK_NULL_HANDLE;
            VmaAllocation vertexAlloc = VK_NULL_HANDLE;
            VkBuffer indexBuffer = VK_NULL_HANDLE;
            VmaAllocation indexAlloc = VK_NULL_HANDLE;
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;
        };

        /// @brief Helper function to stream data to GPU memory.
        /// @details Copies data to GPU memory using non-temporal stores (bypasses CPU cache)
        /// @param void* dst Destination buffer on GPU memory
        /// @param const void* src Source buffer on CPU memory
        /// @param size_t sizeBytes Size of data to copy in bytes
        void StreamToGPU(void* dst, const void* src, size_t sizeBytes);

        VulkanContext& m_r_context;
        Buffer m_Buffer;
        std::vector<std::string> m_Textures;
        int numOfInstances = 0;

        MeshData m_cpuMeshData;
        std::vector<glm::vec3> m_triangleCenters;
        std::vector<uint8_t> m_triangleSubmeshIndices;
        std::vector<SubmeshRange> m_ranges;
        //std::vector<Submesh> m_cpuSubmeshData;
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
