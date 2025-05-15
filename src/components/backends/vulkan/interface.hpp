#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>

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

    class Interface {
    private:
        VulkanContext context;
        bool shouldClose;

        SDL_Window *m_window;

        // Private methods
        void createSwapchain();
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void cleanupSwapchain();
        void recreateSwapchain();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    public:
        Interface(SDL_Window* window);
        ~Interface();

        void bindWindow(SDL_Window* window);
        void unbindWindow();
        void renderFrame();
    };
}

#endif
