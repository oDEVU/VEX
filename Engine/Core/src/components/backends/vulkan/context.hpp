/**
 *  @file   context.hpp
 *  @brief  This file defines struct holding all vulkan data, like device, surface, swapchain, images, views, and more.
 *  @author Eryk Roszkowski
 ***********************************************/

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
#include <components/types.hpp>
#include <string>
#include "components/enviroment.hpp"

namespace vex {
    /// @brief Struct holding all vulkan data, like device, surface, swapchain, images, views, and more.
    /// @todo cleanup
    struct VulkanContext {
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VmaAllocator allocator;
        VkSurfaceKHR surface;

        VkSwapchainKHR swapchain= VK_NULL_HANDLE;
        std::vector<VkImage> swapchainImages;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
        std::vector<VkImageView> swapchainImageViews;

        VkImage depthImage = VK_NULL_HANDLE;
        VmaAllocation depthAllocation = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;
        VkFormat depthFormat;

        VkImage lowResColorImage = VK_NULL_HANDLE;
        VmaAllocation lowResColorAlloc = VK_NULL_HANDLE;
        VkImageView lowResColorView = VK_NULL_HANDLE;
        VkFormat lowResColorFormat = VK_FORMAT_UNDEFINED;

        VkPipelineLayout pipelineLayout;
        //VkCommandPool commandPool;

        std::vector<VkCommandPool> commandPools;
        std::vector<VkCommandBuffer> commandBuffers;

        VkCommandPool singleTimePool;

        /// @brief Function to begin single time command, used for stuff like creating vulkan image when loading textures.
        /// @return VkCommandBuffer
        VkCommandBuffer beginSingleTimeCommands() {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = singleTimePool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);
            return commandBuffer;
        }

        /// @brief Function to end single time command.
        /// @param VkCommandBuffer commandBuffer - The command buffer to end.
        void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);

            vkFreeCommandBuffers(device, singleTimePool, 1, &commandBuffer);
        }

        uint32_t graphicsQueueFamily;
        uint32_t presentQueueFamily;

        uint32_t currentFrame = 0;
        uint32_t currentImageIndex = 0;
        uint32_t MAX_FRAMES_IN_FLIGHT = 3;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout uboDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;

        vex_map<std::string, uint32_t> textureIndices;
        uint32_t nextTextureIndex = 0;

        glm::uvec2 currentRenderResolution;

        enviroment m_enviroment;

        bool supportsMultiDraw = false;
        bool requestSwapchainRecreation = false;
    };
}
