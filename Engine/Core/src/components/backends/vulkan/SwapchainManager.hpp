/**
 *  @file   SwapchainManager.hpp
 *  @brief  This file defines VulkanSwapchainManager Class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <memory>

#include "context.hpp"
#include "components/errorUtils.hpp"

namespace vex {
    /// @brief Manages Vulkan swapchain resources.
    class VulkanSwapchainManager {
    public:
        /// @brief Constructor for VulkanSwapchainManager.
        /// @param VulkanContext& context - Context to store swapchain images/views.
        /// @param SDL_Window* window - Window to query surface capabilities from.
        VulkanSwapchainManager(VulkanContext& context, SDL_Window* window);

        /// @brief Simple destructor.
        ~VulkanSwapchainManager();

        /// @brief Creates or recreates the swapchain.
        /// @details Selects surface format, present mode (FIFO/Mailbox), and extent. Creates Swapchain, Image Views, Depth Resources, and Low-Res offscreen resources.
        void createSwapchain();

        /// @brief Cleans up swapchain resources.
        /// @details Destroys image views, depth buffers, framebuffers, and the swapchain itself.
        void cleanupSwapchain();

        /// @brief Cleans up synchronization objects.
        /// @details Destroys Semaphores (ImageAvailable/RenderFinished) and Fences (InFlight).
        void cleanupSyncObjects();

        /// @brief Recreates the swapchain.
        /// @details Calls `cleanupSwapchain` followed by `createSwapchain`. Typically called on window resize.
        void recreateSwapchain();

        /// @brief Sets the vsync enabled state.
        /// @details Stores the preference. Effect takes place on next `createSwapchain` call (usually triggered via Interface).
        /// @param bool enabled - True for VSync (FIFO), False for Uncapped (Immediate/Mailbox).
        void setVSync(bool enabled);
    private:
        /// @brief Helper function to create image views for swapchain images.
        void createImageViews();

        /// @brief Helper function to create command pool for swapchain creation.
        void createCommandPool();

        /// @brief Helper function to create command buffers for swapchain creation.
        void createCommandBuffers();

        /// @brief Helper function to create synchronization objects for swapchain creation.
        void createSyncObjects();

        /// @brief Helper function to create depth resources for swapchain creation.
        void createDepthResources();

        /// @brief Helper function to create low resolution resources for low res rendering.
        void createLowResResources();

        /// @brief Helper function to cleanup low resolution resources, since low res resources can be changed without changing window resiolution.
        /// @see ResolutionManager
        void cleanupLowResResources();

        /// @brief Helper function choosing SwapSurfaceFormat based on hardware capabilities.
        /// @param const std::vector<VkSurfaceFormatKHR>& availableFormats
        /// @return VkSurfaceFormatKHR.
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

        /// @brief Helper function choosing SwapPresentMode based on hardware capabilities.
        /// @param const std::vector<VkPresentModeKHR>& availablePresentModes
        /// @return VkPresentModeKHR.
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

        /// @brief Helper function choosing SwapExtent based on hardware capabilities.
        /// @param const VkSurfaceCapabilitiesKHR& capabilities
        /// @return VkExtent2D.
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        /// @brief Helper function to find depth format based on hardware capabilities.
        /// @return VkFormat.
        VkFormat findDepthFormat();

        VulkanContext& m_r_context;
        SDL_Window *m_p_window;
        bool m_vsyncEnabled = false;
    };
}
