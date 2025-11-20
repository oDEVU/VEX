#include "SwapchainManager.hpp"

#include <vector>
#include <algorithm>
#include <set>
#include <limits>
#include <cassert>
#include <array>
#include "limits.hpp"

namespace vex {
    VulkanSwapchainManager::VulkanSwapchainManager(VulkanContext& context, SDL_Window* window) : m_r_context(context) {
        m_p_window = window;
    }
    VulkanSwapchainManager::~VulkanSwapchainManager() {}

    void VulkanSwapchainManager::createSwapchain() {
        log("Creating Swapchain");
        VkSurfaceCapabilitiesKHR capabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_r_context.physicalDevice, m_r_context.surface, &capabilities) != VK_SUCCESS) {
            throw_error("Failed to get surface capabilities");
        }

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_r_context.physicalDevice, m_r_context.surface, &formatCount, nullptr);
        if (formatCount == 0) {
            throw_error("No surface formats supported");
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_r_context.physicalDevice, m_r_context.surface, &formatCount, formats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_r_context.physicalDevice, m_r_context.surface, &presentModeCount, nullptr);
        if (presentModeCount == 0) {
            throw_error("No present modes supported");
        }

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_r_context.physicalDevice, m_r_context.surface, &presentModeCount, presentModes.data());

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        VkExtent2D extent = chooseSwapExtent(capabilities);

        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(m_r_context.physicalDevice, surfaceFormat.format, &formatProps);

        if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
            !(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
            throw_error("Swapchain format doesn't support blitting!");
        }

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_r_context.surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        uint32_t queueFamilyIndices[] = {m_r_context.graphicsQueueFamily, m_r_context.presentQueueFamily};
        if (m_r_context.graphicsQueueFamily != m_r_context.presentQueueFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_r_context.device, &createInfo, nullptr, &m_r_context.swapchain) != VK_SUCCESS) {
            throw_error("Failed to create swapchain");
        }

        vkGetSwapchainImagesKHR(m_r_context.device, m_r_context.swapchain, &imageCount, nullptr);
        m_r_context.swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_r_context.device, m_r_context.swapchain, &imageCount, m_r_context.swapchainImages.data());

        m_r_context.swapchainImageFormat = surfaceFormat.format;
        m_r_context.swapchainExtent = extent;

        createImageViews();

        VkCommandBuffer cmd = m_r_context.beginSingleTimeCommands();

        for (auto& image : m_r_context.swapchainImages) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }

        m_r_context.endSingleTimeCommands(cmd);
    }

    void VulkanSwapchainManager::createDepthResources() {
        log("Creating depth resources...");

        VkFormat depthFormat = findDepthFormat();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {m_r_context.swapchainExtent.width, m_r_context.swapchainExtent.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (vmaCreateImage(m_r_context.allocator, &imageInfo, &allocInfo,
                          &m_r_context.depthImage, &m_r_context.depthAllocation, nullptr) != VK_SUCCESS) {
            throw_error("Failed to create depth image");
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_r_context.depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_r_context.device, &viewInfo, nullptr, &m_r_context.depthImageView) != VK_SUCCESS) {
            throw_error("Failed to create depth image view");
        }

        m_r_context.depthFormat = depthFormat;

        createCommandPool();
    }

    void VulkanSwapchainManager::createImageViews() {
        log("creating imageviews");
        m_r_context.swapchainImageViews.resize(m_r_context.swapchainImages.size());

        for (size_t i = 0; i < m_r_context.swapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_r_context.swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_r_context.swapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_r_context.device, &createInfo, nullptr, &m_r_context.swapchainImageViews[i]) != VK_SUCCESS) {
                throw_error("Failed to create image views");
            }
        }

        createDepthResources();
    }

    void VulkanSwapchainManager::createCommandPool() {
        log("creating command pools");

        m_r_context.commandPools.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < m_r_context.MAX_FRAMES_IN_FLIGHT; i++) {
            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = m_r_context.graphicsQueueFamily;

            if (vkCreateCommandPool(m_r_context.device, &poolInfo, nullptr, &m_r_context.commandPools[i]) != VK_SUCCESS) {
                throw_error("Failed to create command pool");
            }
        }

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                         VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_r_context.graphicsQueueFamily;

        if (vkCreateCommandPool(m_r_context.device, &poolInfo, nullptr, &m_r_context.singleTimePool) != VK_SUCCESS) {
            throw_error("Failed to create single time command pool");
        }
        //m_r_context.singleTimePool = createCommandPool(m_r_context.device, m_r_context.graphicsQueueFamily);

        createCommandBuffers();
    }

    void VulkanSwapchainManager::createCommandBuffers() {
        log("creating command buffers");

        m_r_context.commandBuffers.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < m_r_context.MAX_FRAMES_IN_FLIGHT; i++) {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = m_r_context.commandPools[i];
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(m_r_context.device, &allocInfo, &m_r_context.commandBuffers[i]) != VK_SUCCESS) {
                throw_error("Failed to allocate command buffer");
            }
        }

        createSyncObjects();
    }

    void VulkanSwapchainManager::createSyncObjects() {
        log("Creating synchronization objects...");

        m_r_context.imageAvailableSemaphores.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);
        m_r_context.renderFinishedSemaphores.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);
        m_r_context.inFlightFences.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult result;

        for (size_t i = 0; i < m_r_context.MAX_FRAMES_IN_FLIGHT; i++) {
            result = vkCreateSemaphore(m_r_context.device, &semaphoreInfo, nullptr, &m_r_context.imageAvailableSemaphores[i]);
            if (result != VK_SUCCESS) {
                throw_error("Failed to create image available semaphore");
            }

            result = vkCreateSemaphore(m_r_context.device, &semaphoreInfo, nullptr, &m_r_context.renderFinishedSemaphores[i]);
            if (result != VK_SUCCESS) {
                throw_error("Failed to create render finished semaphore");
            }

            result = vkCreateFence(m_r_context.device, &fenceInfo, nullptr, &m_r_context.inFlightFences[i]);
            if (result != VK_SUCCESS) {
                throw_error("Failed to create fence");
            }

            assert(m_r_context.inFlightFences[i] != VK_NULL_HANDLE);

            log("Created sync objects: fence=%p", (void*)m_r_context.inFlightFences[i]);
        }

        if (m_r_context.depthImageView) {
            createLowResResources();
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                       "Deferring low-res resource creation - depth image not ready");
        }
    }

    void VulkanSwapchainManager::createLowResResources() {
        if (m_r_context.currentRenderResolution.x == 0 || m_r_context.currentRenderResolution.y == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid render resolution for low-res resources");
            return;
        }

        cleanupLowResResources();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = m_r_context.swapchainImageFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (vmaCreateImage(m_r_context.allocator, &imageInfo, &allocInfo,
                          &m_r_context.lowResColorImage, &m_r_context.lowResColorAlloc, nullptr) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create low-res color image");
            return;
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_r_context.lowResColorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_r_context.swapchainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_r_context.device, &viewInfo, nullptr, &m_r_context.lowResColorView) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create low-res color image view");
            vmaDestroyImage(m_r_context.allocator, m_r_context.lowResColorImage, m_r_context.lowResColorAlloc);
            m_r_context.lowResColorImage = VK_NULL_HANDLE;
            m_r_context.lowResColorAlloc = VK_NULL_HANDLE;
            return;
        }

        m_r_context.lowResColorFormat = m_r_context.swapchainImageFormat;
    }

    void VulkanSwapchainManager::cleanupLowResResources() {
        if (m_r_context.lowResColorView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_r_context.device, m_r_context.lowResColorView, nullptr);
            m_r_context.lowResColorView = VK_NULL_HANDLE;
        }

        if (m_r_context.lowResColorImage != VK_NULL_HANDLE && m_r_context.lowResColorAlloc != VK_NULL_HANDLE) {
            vmaDestroyImage(m_r_context.allocator, m_r_context.lowResColorImage, m_r_context.lowResColorAlloc);
            m_r_context.lowResColorImage = VK_NULL_HANDLE;
            m_r_context.lowResColorAlloc = VK_NULL_HANDLE;
        }
    }

    void VulkanSwapchainManager::cleanupSwapchain() {
                    if (m_r_context.lowResColorView) {
                        vkDestroyImageView(m_r_context.device, m_r_context.lowResColorView, nullptr);
                    }
                    if (m_r_context.lowResColorImage) {
                        vmaDestroyImage(m_r_context.allocator, m_r_context.lowResColorImage, m_r_context.lowResColorAlloc);
                    }

        if (m_r_context.depthImageView) {
            vkDestroyImageView(m_r_context.device, m_r_context.depthImageView, nullptr);
            m_r_context.depthImageView = VK_NULL_HANDLE;
        }
        if (m_r_context.depthImage) {
            vmaDestroyImage(m_r_context.allocator, m_r_context.depthImage, m_r_context.depthAllocation);
            m_r_context.depthImage = VK_NULL_HANDLE;
        }

        for (auto& imageView : m_r_context.swapchainImageViews) {
            vkDestroyImageView(m_r_context.device, imageView, nullptr);
        }
        m_r_context.swapchainImageViews.clear();

        vkDestroySwapchainKHR(m_r_context.device, m_r_context.swapchain, nullptr);
        m_r_context.swapchain = VK_NULL_HANDLE;
    }

    void VulkanSwapchainManager::recreateSwapchain() {
        log("recreating swapchains");
        vkDeviceWaitIdle(m_r_context.device);
        log("cleanupLowResResources");
        cleanupLowResResources();
        log("cleanupSwapchain");
        cleanupSwapchain();

        createSwapchain();
        log("createImageViews");
        createImageViews();
        log("createCommandBuffers");
        createCommandPool();
        log("createLowResResources");
        createLowResResources();
    }

    VkSurfaceFormatKHR VulkanSwapchainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        log("choosing swapsurface format");
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        log("choosing swap present mode");

        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                if (SDL_GetCurrentVideoDriver() && strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
                    log("Wayland detected, FIFO selected");
                    return VK_PRESENT_MODE_FIFO_KHR;
                }else{
                    return availablePresentMode;
                }
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        log("choosing swap extent");
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            SDL_GetWindowSizeInPixels(m_p_window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(m_r_context.currentRenderResolution.x),
                static_cast<uint32_t>(m_r_context.currentRenderResolution.y)
            };

            actualExtent.width = std::clamp(
                actualExtent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(
                actualExtent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    VkFormat VulkanSwapchainManager::findDepthFormat() {
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_r_context.physicalDevice, format, &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                return format;
            }
        }
        throw_error("Failed to find supported depth format");
        return VK_FORMAT_UNDEFINED;
    }
}
