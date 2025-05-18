#pragma once


#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <memory>

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

        // Queue family indices
        uint32_t graphicsQueueFamily;
        uint32_t presentQueueFamily;

        // Frame management
        uint32_t currentFrame = 0;
        uint32_t currentImageIndex = 0;
        const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        // Synchronization (expanded)
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        VkDescriptorSetLayout descriptorSetLayout;
    } VulkanContext;
}
