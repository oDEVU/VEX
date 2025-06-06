#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <memory>

//VEX components
#include "context.hpp"
#include "components/errorUtils.hpp"

namespace vex {
    class VulkanSwapchainManager {
    public:
        explicit VulkanSwapchainManager(VulkanContext& context, SDL_Window* window);
        ~VulkanSwapchainManager();

        void createSwapchain();
        void cleanupSwapchain();
        void recreateSwapchain();

    private:
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void createDepthResources();
        void createLowResResources();

        void cleanupLowResResources();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        VkFormat findDepthFormat();

        VulkanContext& context_;
        SDL_Window *m_window;
    };
}
