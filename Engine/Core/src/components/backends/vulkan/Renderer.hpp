/**
 *  @file   Renderer.hpp
 *  @brief  This file defines Renderer Class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include "components/GameComponents/BasicComponents.hpp"
#include "context.hpp"
#include "Resources.hpp"
#include "Pipeline.hpp"
#include "SwapchainManager.hpp"
#include "VulkanImGUIWrapper.hpp"
#include "VulkanMesh.hpp"
#include "components/UI/VexUI.hpp"
#include "MeshManager.hpp"
#include "PhysicsDebug.hpp"
#include "entt/entity/fwd.hpp"
#include <glm/glm.hpp>
#include <chrono>
#include <components/GameComponents/UiComponent.hpp>

namespace vex {
    /// @brief Struct to hold information about a render item.
    struct RenderItem {
        entt::entity entity;
        uint32_t modelIndex;
    };

    /// @brief Data structure to pass state between render stages
        struct SceneRenderData {
            VkCommandBuffer commandBuffer;
            uint32_t frameIndex;
            uint32_t imageIndex;
            VkDescriptorSet imguiTextureID;
            bool isSwapchainValid;
        };

    /// @brief Class responsible for rendering the scene using Vulkan backend.
    class Renderer {
    public:
        /// @brief Constructor for Renderer class.
        /// @details Initializes rendering resources such as the screen sampler, descriptor pools, and debug/indirect buffers if supported.
        /// @param VulkanContext& context - The Vulkan context.
        /// @param std::unique_ptr<VulkanResources>& resources - Resource manager for textures/buffers.
        /// @param std::unique_ptr<VulkanPipeline>& pipeline - Standard Opaque pipeline.
        /// @param std::unique_ptr<VulkanPipeline>& transPipeline - Transparent pipeline.
        /// @param std::unique_ptr<VulkanPipeline>& maskPipeline - Masked pipeline (alpha cutout).
        /// @param std::unique_ptr<VulkanPipeline>& uiPipeline - User Interface pipeline.
        /// @param std::unique_ptr<VulkanPipeline>& fullscreenPipeline - Fullscreen/Post-process pipeline.
        /// @param std::unique_ptr<VulkanSwapchainManager>& swapchainManager - Swapchain manager.
        /// @param std::unique_ptr<MeshManager>& meshManager - Manager for mesh buffers.
        Renderer(VulkanContext& context,
                 std::unique_ptr<VulkanResources>& resources,
                 std::unique_ptr<VulkanPipeline>& pipeline,
                 std::unique_ptr<VulkanPipeline>& transPipeline,
                 std::unique_ptr<VulkanPipeline>& maskPipeline,
                 std::unique_ptr<VulkanPipeline>& uiPipeline,
                 std::unique_ptr<VulkanPipeline>& fullscreenPipeline,
                 std::unique_ptr<VulkanSwapchainManager>& swapchainManager,
                 std::unique_ptr<MeshManager>& meshManager);

        /// @brief Destructor for Renderer class.
        /// @details Cleans up samplers, descriptor pools, and buffers (debug/indirect).
        ~Renderer();

        /// @brief Prepares the frame for rendering.
        /// @details handles Swapchain recreation (if resolution changed), waits for fences, acquires the next image, and begins the command buffer.
        /// @param glm::uvec2 renderResolution - The desired resolution for the low-res render target.
        /// @param SceneRenderData& outData - Output struct filled with current frame context (cmd buffer, frame index).
        /// @return bool - True if frame acquisition was successful, False if swapchain needs regeneration.
        bool beginFrame(glm::uvec2 renderResolution, SceneRenderData& outData);

        /// @brief Renders the 3D scene and UI to an offscreen low-res texture.
        /// @details
        /// 1. Updates Scene UBO (View/Proj, PS1 effects like jitter/snapping).
        /// 2. Performs frustum culling on objects.
        /// 3. Updates Light UBOs for visible objects.
        /// 4. Sorts objects into Opaque, Masked, and Transparent queues.
        /// 5. Executes draw calls (using MultiDraw for transparency if supported).
        /// 6. Renders UI components on top.
        /// @param SceneRenderData& data - Frame context data.
        /// @param const entt::entity cameraEntity - The active camera entity.
        /// @param entt::registry& registry - ECS registry to query objects.
        /// @param int frame - Current frame number (used for animations/logic).
        /// @param const std::vector<DebugVertex>* debugLines - Optional debug lines to draw.
        /// @param bool isEditorMode - If true, applies editor-specific logic (e.g. gizmos, override camera).
        void renderScene(SceneRenderData& data,
                         const entt::entity cameraEntity,
                         entt::registry& registry,
                         int frame,
                         const std::vector<DebugVertex>* debugLines = nullptr,
                         bool isEditorMode = false);

        /// @brief Gets or creates a cached ImGui texture descriptor for the scene.
        /// @details Used to display the rendered scene inside an ImGui window (Editor Viewport).
        /// @param ImGUIWrapper& ui - Reference to the UI wrapper to register the texture.
        /// @return VkDescriptorSet - The descriptor set handle for ImGui.
        VkDescriptorSet getImGuiTextureID(ImGUIWrapper& ui);

        /// @brief Composes the final frame onto the Swapchain image.
        /// @details
        /// - **Game Mode**: Draws a fullscreen quad to upscale the low-res scene texture to the window size.
        /// - **Editor Mode**: Renders the ImGui overlay which contains the scene view.
        /// @param SceneRenderData& data - Frame context data.
        /// @param ImGUIWrapper& ui - UI wrapper for drawing.
        /// @param bool isEditorMode - Toggles between Fullscreen Quad or ImGui composition.
        void composeFrame(SceneRenderData& data,
                          ImGUIWrapper& ui,
                          bool isEditorMode);

        /// @brief Submits the command buffer and presents the image.
        /// @details Submits to the Graphics Queue and calls `vkQueuePresentKHR`. Handles `VK_ERROR_OUT_OF_DATE_KHR` by triggering swapchain recreation.
        /// @param SceneRenderData& data - Frame context data.
        void endFrame(SceneRenderData& data);

        #if DEBUG
            /// @brief Sets the debug pipeline used for wireframe/line rendering.
            /// @param std::unique_ptr<VulkanPipeline>* pipeline - Pointer to the pipeline pointer.
            void setDebugPipeline(std::unique_ptr<VulkanPipeline>* pipeline) { m_pp_debugPipeline = pipeline; }

            /// @brief Renders debug lines.
            /// @details Updates the debug vertex buffer and issues a draw call.
            /// @param VkCommandBuffer cmd - The command buffer.
            /// @param int frameIndex - Current frame index.
            /// @param const std::vector<DebugVertex>& lines - Vertices to draw.
            void renderDebug(VkCommandBuffer cmd, int frameIndex, const std::vector<DebugVertex>& lines);
        #endif

    private:
        /// @brief Helper function for image transition
        /// @param VkCommandBuffer cmd - Command buffer.
        /// @param VkImage image - Image.
        /// @param VkImageLayout oldLayout - Old layout.
        /// @param VkImageLayout newLayout - New layout.
        /// @param VkAccessFlags srcAccessMask - Source access mask.
        /// @param VkAccessFlags dstAccessMask - Destination access mask.
        /// @param VkPipelineStageFlags srcStage - Source stage.
        /// @param VkPipelineStageFlags dstStage - Destination stage.
        /// @details It is used internally to transition low resolution image to window resolution.
        void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

        /// @brief Issues a multi-draw indexed command used by transparent meshes since they are drawn triangle by triangle.
        /// @param VkCommandBuffer cmd - Command buffer.
        /// @param const std::vector<VkMultiDrawIndexedInfoEXT>& commands - Vector of multi-draw indexed commands.
        void issueMultiDrawIndexed(VkCommandBuffer cmd, const std::vector<VkMultiDrawIndexedInfoEXT>& commands);

        /// @brief Updates the screen descriptor.
        /// @param VkImageView view - Image view.
        void updateScreenDescriptor(VkImageView view);

        VulkanContext& m_r_context;
        std::unique_ptr<VulkanResources>& m_p_resources;
        std::unique_ptr<VulkanPipeline>& m_p_pipeline;
        std::unique_ptr<VulkanPipeline>& m_p_transPipeline;
        std::unique_ptr<VulkanPipeline>& m_p_maskPipeline;
        std::unique_ptr<VulkanPipeline>& m_p_uiPipeline;
        std::unique_ptr<VulkanPipeline>& m_p_fullscreenPipeline;
        std::unique_ptr<VulkanSwapchainManager>& m_p_swapchainManager;
        std::unique_ptr<MeshManager>& m_p_meshManager;

        std::chrono::high_resolution_clock::time_point startTime;
        float currentTime = 0.0f;

        bool basicDiag = true;

        std::vector<TransparentTriangle> m_transparentTriangles;
        std::map<uint32_t, glm::mat4> trnasMatrixes;
        std::vector<UiComponent> m_uiObjects;
        std::vector<VkMultiDrawIndexedInfoEXT> m_multiDrawInfos;
        size_t approxTriangles = 0;

        SceneUBO m_sceneUBO;

        VkDescriptorSet m_screenDescriptorSet = VK_NULL_HANDLE;
        VkSampler m_screenSampler = VK_NULL_HANDLE;
        VkImageView m_lastUsedView = VK_NULL_HANDLE;
        VkDescriptorSet m_cachedImGuiDescriptor = VK_NULL_HANDLE;
        std::vector<std::vector<VkDescriptorSet>> m_garbageDescriptors;
        VkDescriptorPool m_localPool = VK_NULL_HANDLE;

        std::vector<RenderItem> opaqueQueue;
        std::vector<RenderItem> maskedQueue;

        std::vector<VkBuffer> m_indirectBuffers;
        std::vector<VmaAllocation> m_indirectAllocations;

        #if DEBUG
            std::vector<VkBuffer> m_debugBuffers;
            std::vector<VmaAllocation> m_debugAllocations;
            std::unique_ptr<VulkanPipeline>* m_pp_debugPipeline = nullptr;
            std::unique_ptr<VulkanMesh> m_editorCameraVulkanMesh;
            MeshData m_editorCameraMesh;
        #endif
    };
}
