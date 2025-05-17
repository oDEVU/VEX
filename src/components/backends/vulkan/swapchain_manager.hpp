#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <memory>

//Renderer
#include "context.hpp"

namespace vex {
    class VulkanSwapchainManager {
    public:
        explicit VulkanSwapchainManager(VulkanContext& context, SDL_Window* window);
        ~VulkanSwapchainManager();

        void createSwapchain();
        void cleanupSwapchain();
        void recreateSwapchain();

    private:
        // Helper methods (formerly private in VulkanInterface)
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();

        // Swapchain utilities
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VulkanContext& context_;
        SDL_Window *m_window;
    };
}
