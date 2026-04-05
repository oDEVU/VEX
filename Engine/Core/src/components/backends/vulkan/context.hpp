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
#include <components/Types.hpp>
#include <string>
#include <queue>
#include "components/Environment.hpp"

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

        VkImage compositeImage = VK_NULL_HANDLE;
        VmaAllocation compositeAlloc = VK_NULL_HANDLE;
        VkImageView compositeView = VK_NULL_HANDLE;

        VkImage uiImage = VK_NULL_HANDLE;
        VmaAllocation uiAlloc = VK_NULL_HANDLE;
        VkImageView uiView = VK_NULL_HANDLE;
        VkFormat lowResColorFormat = VK_FORMAT_UNDEFINED;

        VkImage accumImage = VK_NULL_HANDLE;
        VmaAllocation accumAlloc = VK_NULL_HANDLE;
        VkImageView accumView = VK_NULL_HANDLE;

        VkImage revealImage = VK_NULL_HANDLE;
        VmaAllocation revealAlloc = VK_NULL_HANDLE;
        VkImageView revealView = VK_NULL_HANDLE;

        VkImage gameViewImage = VK_NULL_HANDLE;
        VmaAllocation gameViewAlloc = VK_NULL_HANDLE;
        VkImageView gameViewView = VK_NULL_HANDLE;

        VkImage colorLutImage = VK_NULL_HANDLE;
        VmaAllocation colorLutAllocation = VK_NULL_HANDLE;
        VkImageView colorLutView = VK_NULL_HANDLE;

        VkPipelineLayout pipelineLayout;

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

        /// @brief Current frame index for triple-buffering
        /// @details This wraps around at MAX_FRAMES_IN_FLIGHT. Always validate bounds
        /// before using it to index into frame-dependent arrays.
        uint32_t currentFrame = 0;
        
        /// @brief Current swapchain image index (acquired from vkAcquireNextImageKHR)
        /// @details Should be validated against swapchainImages.size() before use
        uint32_t currentImageIndex = 0;
        
        /// @brief Maximum frames in flight (triple-buffering)
        /// @details This is set dynamically during swapchain creation.
        /// Always use for bounds checking: index < MAX_FRAMES_IN_FLIGHT
        uint32_t MAX_FRAMES_IN_FLIGHT = 3;

        /// @brief Synchronization semaphores - must be sized to MAX_FRAMES_IN_FLIGHT
        /// @details Array size must equal MAX_FRAMES_IN_FLIGHT. Validate index bounds before access.
        std::vector<VkSemaphore> imageAvailableSemaphores;
        
        /// @brief Render finished semaphores - must be sized to MAX_FRAMES_IN_FLIGHT
        /// @details Array size must equal MAX_FRAMES_IN_FLIGHT. Validate index bounds before access.
        std::vector<VkSemaphore> renderFinishedSemaphores;
        
        /// @brief In-flight fences - must be sized to MAX_FRAMES_IN_FLIGHT
        /// @details Array size must equal MAX_FRAMES_IN_FLIGHT. Validate index bounds before access.
        std::vector<VkFence> inFlightFences;

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout uboDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout screenDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout particleDescriptorSetLayout = VK_NULL_HANDLE;

        /// @brief Validates a frame index against MAX_FRAMES_IN_FLIGHT
        /// @param frameIndex - The frame index to validate
        /// @return true if frameIndex is valid (< MAX_FRAMES_IN_FLIGHT), false otherwise
        bool isValidFrameIndex(uint32_t frameIndex) const {
            if (frameIndex >= MAX_FRAMES_IN_FLIGHT) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                           "Invalid frame index %u (MAX_FRAMES_IN_FLIGHT: %u)",
                           frameIndex, MAX_FRAMES_IN_FLIGHT);
                return false;
            }
            return true;
        }

        /// @brief Validates an image index against swapchain image count
        /// @param imageIndex - The image index to validate
        /// @return true if imageIndex is valid (< swapchainImages.size()), false otherwise
        bool isValidImageIndex(uint32_t imageIndex) const {
            if (imageIndex >= swapchainImages.size()) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                           "Invalid image index %u (swapchain image count: %zu)",
                           imageIndex, swapchainImages.size());
                return false;
            }
            return true;
        }

        /// @brief Asserts that all frame-dependent synchronization vectors are properly sized
        /// @details Should be called after swapchain creation to verify consistency
        void validateSyncObjectSizes() const {
            assert(imageAvailableSemaphores.size() == MAX_FRAMES_IN_FLIGHT &&
                   "imageAvailableSemaphores size mismatch");
            assert(renderFinishedSemaphores.size() == MAX_FRAMES_IN_FLIGHT &&
                   "renderFinishedSemaphores size mismatch");
            assert(inFlightFences.size() == MAX_FRAMES_IN_FLIGHT &&
                   "inFlightFences size mismatch");
            assert(commandPools.size() == MAX_FRAMES_IN_FLIGHT &&
                   "commandPools size mismatch");
            assert(commandBuffers.size() == MAX_FRAMES_IN_FLIGHT &&
                   "commandBuffers size mismatch");
        }

        vex_map<std::string, uint32_t> textureIndices;
        uint32_t nextTextureIndex = 0;

        std::queue<uint32_t> recycledTextureIndices;

        glm::uvec2 currentRenderResolution;

        environment m_environment;

        bool supportsMultiDraw = false;
        bool supportsIndirectDraw = false;
        bool supportsBindlessTextures = false;
        bool supportsShaderDrawParameters = false;

        VkDescriptorSetLayout bindlessDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool bindlessDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet bindlessDescriptorSet = VK_NULL_HANDLE;

        uint32_t maxDrawIndirectCount = 1;
        uint32_t maxMultiDrawCount = 1;
        bool requestSwapchainRecreation = false;

        uint32_t vulkanVersion = 0;
    };
}
