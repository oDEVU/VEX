#pragma once

#include "context.hpp"
#include "Resources.hpp"
#include "Pipeline.hpp"
#include "SwapchainManager.hpp"
#include "VulkanImGUIWrapper.hpp"
#include "VulkanMesh.hpp"
#include "components/UI/VexUI.hpp"
#include "MeshManager.hpp"
#include <glm/glm.hpp>
#include <chrono>

namespace vex {
    class Renderer {
    public:
        Renderer(VulkanContext& context,
                 std::shared_ptr<VulkanResources>& resources,
                 std::unique_ptr<VulkanPipeline>& pipeline,
                 std::unique_ptr<VulkanPipeline>& uiPipeline,
                 std::unique_ptr<VulkanSwapchainManager>& swapchainManager,
                 std::unique_ptr<MeshManager>& meshManager);
        ~Renderer();

        void renderFrame(const glm::mat4& view, const glm::mat4& proj, glm::uvec2 renderResolution, entt::registry& registry, ImGUIWrapper& ui, VexUI& vui, uint64_t frame);

    private:
        void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

        VulkanContext& m_r_context;
        std::shared_ptr<VulkanResources>& m_p_resources;
        std::unique_ptr<VulkanPipeline>& m_p_pipeline;
        std::unique_ptr<VulkanPipeline>& m_p_uiPipeline;
        std::unique_ptr<VulkanSwapchainManager>& m_p_swapchainManager;
        std::unique_ptr<MeshManager>& m_p_meshManager;
        std::chrono::high_resolution_clock::time_point startTime;
        float currentTime = 0.0f;
    };
}
