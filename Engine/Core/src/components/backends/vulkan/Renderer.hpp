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
#include "entt/entity/fwd.hpp"
#include <glm/glm.hpp>
#include <chrono>
#include <components/GameComponents/UiComponent.hpp>

namespace vex {
    /// @brief Class responsible for rendering the scene using Vulkan backend.
    class Renderer {
    public:
        /// @brief Constructor for Renderer class.
        /// @param std::shared_ptr<VulkanResources>& resources - Vulkan resources needed for textures std::map.
        /// @param std::unique_ptr<VulkanPipeline>& pipeline - VulkanPipeline.
        /// @param std::unique_ptr<VulkanPipeline>& uiPipeline - VulkanPipeline for UI rendering.
        /// @param std::unique_ptr<VulkanSwapchainManager>& swapchainManager - VulkanSwapchainManager.
        /// @param std::unique_ptr<MeshManager>& meshManager - MeshManager.
        /// @details Its created and handled by VulkanInteface constructor.
        Renderer(VulkanContext& context,
                 std::unique_ptr<VulkanResources>& resources,
                 std::unique_ptr<VulkanPipeline>& pipeline,
                 std::unique_ptr<VulkanPipeline>& uiPipeline,
                 std::unique_ptr<VulkanSwapchainManager>& swapchainManager,
                 std::unique_ptr<MeshManager>& meshManager);

        /// @brief Destructor for Renderer class.
        ~Renderer();

        /// @brief Renders a frame using the provided parameters.
        /// @param const glm::mat4& view - View matrix.
        /// @param const glm::mat4& proj - Projection matrix.
        /// @param glm::uvec2 renderResolution - Render resolution.
        /// @param entt::registry& registry - Entity registry.
        /// @param ImGUIWrapper& ui - ImGUI wrapper.
        /// @param uint64_t frame - Frame number.
        void renderFrame(const entt::entity cameraEntity, glm::uvec2 renderResolution, entt::registry& registry, ImGUIWrapper& ui, uint64_t frame);

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

        void issueMultiDrawIndexed(VkCommandBuffer cmd, const std::vector<VkMultiDrawIndexedInfoEXT>& commands);

        VulkanContext& m_r_context;
        std::unique_ptr<VulkanResources>& m_p_resources;
        std::unique_ptr<VulkanPipeline>& m_p_pipeline;
        std::unique_ptr<VulkanPipeline>& m_p_uiPipeline;
        std::unique_ptr<VulkanSwapchainManager>& m_p_swapchainManager;
        std::unique_ptr<MeshManager>& m_p_meshManager;
        std::chrono::high_resolution_clock::time_point startTime;
        float currentTime = 0.0f;
        std::vector<TransparentTriangle> m_transparentTriangles;
        std::vector<UiComponent> m_uiObjects;
        std::vector<VkMultiDrawIndexedInfoEXT> m_multiDrawInfos;
        size_t approxTriangles = 0;


        SceneUBO m_sceneUBO;
    };
}
