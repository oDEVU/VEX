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
        /// @param std::shared_ptr<VulkanResources>& resources - Vulkan resources needed for textures std::map.
        /// @param std::unique_ptr<VulkanPipeline>& pipeline - VulkanPipeline.
        /// @param std::unique_ptr<VulkanPipeline>& transPipeline - VulkanPipeline for transparent objects.
        /// @param std::unique_ptr<VulkanPipeline>& maskPipeline - VulkanPipeline for masked objects.
        /// @param std::unique_ptr<VulkanPipeline>& uiPipeline - VulkanPipeline for UI rendering.
        /// @param std::unique_ptr<VulkanPipeline>& fullscreenPipeline - VulkanPipeline for fullscreen effects.
        /// @param std::unique_ptr<VulkanSwapchainManager>& swapchainManager - VulkanSwapchainManager.
        /// @param std::unique_ptr<MeshManager>& meshManager - MeshManager.
        /// @details Its created and handled by VulkanInteface constructor.
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
        ~Renderer();

        /// @brief Prepares frame, acquires image, handles resize
        /// @param glm::uvec2 renderResolution - Resolution of the render target.
        /// @param SceneRenderData& outData - Output data for the frame.
        /// @return bool - True if the frame was prepared successfully, false otherwise.
        bool beginFrame(glm::uvec2 renderResolution, SceneRenderData& outData);

        /// @brief Renders 3D scene and VexUI to low-res texture
        /// @param SceneRenderData& data - Data for the frame.
        /// @param const entt::entity cameraEntity - Entity representing the camera.
        /// @param entt::registry& registry - Registry containing entities and components.
        void renderScene(SceneRenderData& data,
                         const entt::entity cameraEntity,
                         entt::registry& registry,
                         int frame,
                         const std::vector<DebugVertex>* debugLines = nullptr,
                         bool isEditorMode = false);

        /// @brief Get the ImGui texture ID
        /// @param ImGUIWrapper& ui - ImGUI wrapper.
        /// @return VkDescriptorSet - Descriptor set for the ImGui texture.
        VkDescriptorSet getImGuiTextureID(ImGUIWrapper& ui);

        /// @brief Composite to screen (Fullscreen Quad or ImGui)
        /// @param SceneRenderData& data - Data for the frame.
        /// @param ImGUIWrapper& ui - ImGUI wrapper.
        /// @param bool isEditorMode - Whether the editor mode is active.
        void composeFrame(SceneRenderData& data,
                          ImGUIWrapper& ui,
                          bool isEditorMode);

        /// @brief Submits and Presents
        /// @param SceneRenderData& data - Data for the frame.
        void endFrame(SceneRenderData& data);

        #if DEBUG
            void setDebugPipeline(std::unique_ptr<VulkanPipeline>* pipeline) { m_pp_debugPipeline = pipeline; }
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
