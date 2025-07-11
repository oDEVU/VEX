#pragma once


#include <sys/types.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <unordered_map>
#include <components/Model.hpp>

namespace vex {
    typedef struct {
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VmaAllocator allocator;
        VkSurfaceKHR surface;

        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchainImages;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
        std::vector<VkImageView> swapchainImageViews;

        VkImage depthImage = VK_NULL_HANDLE;
        VmaAllocation depthAllocation = VK_NULL_HANDLE;
        VkImageView depthImageView;
        VkFormat depthFormat;

        VkImage lowResColorImage = VK_NULL_HANDLE;
        VmaAllocation lowResColorAlloc = VK_NULL_HANDLE;
        VkImageView lowResColorView = VK_NULL_HANDLE;
        VkFormat lowResColorFormat = VK_FORMAT_UNDEFINED;

        VkPipelineLayout pipelineLayout;
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;

        VkCommandBuffer beginSingleTimeCommands() {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);
            return commandBuffer;
        }

        void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);

            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }

        uint32_t graphicsQueueFamily;
        uint32_t presentQueueFamily;

        uint32_t currentFrame = 0;
        uint32_t currentImageIndex = 0;
        const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        //FIXME: Implement somethig to dynamically allocate resources and not relly on max txt and models
        const uint32_t MAX_TEXTURES = 1024;
        const uint32_t MAX_MODELS = 1024;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSetLayout uboDescriptorSetLayout;
        VkDescriptorSetLayout textureDescriptorSetLayout;

        std::unordered_map<std::string, uint32_t> textureIndices;
        uint32_t nextTextureIndex = 0;

        glm::uvec2 currentRenderResolution;
    } VulkanContext;
}
