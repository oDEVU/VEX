#pragma once

namespace vex {
    typedef struct {
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VmaAllocator allocator;
        VkSurfaceKHR surface;

        // Swapchain members
        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchainImages;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
        std::vector<VkImageView> swapchainImageViews;

        // Rendering members
        VkRenderPass renderPass;
        std::vector<VkFramebuffer> swapchainFramebuffers;
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;

        // Synchronization
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;

        // Queue family indices
        uint32_t graphicsQueueFamily;
        uint32_t presentQueueFamily;
    } VulkanContext;
}
