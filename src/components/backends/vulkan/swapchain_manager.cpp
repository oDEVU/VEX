#include "swapchain_manager.hpp"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <limits>

namespace vex {
    VulkanSwapchainManager::VulkanSwapchainManager(VulkanContext& context, SDL_Window* window) : context_(context) {
        m_window = window;
    }
    VulkanSwapchainManager::~VulkanSwapchainManager() {}

    void VulkanSwapchainManager::createSwapchain() {
        SDL_Log("Creating Swapchain");
        // Query swapchain support
        VkSurfaceCapabilitiesKHR capabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context_.physicalDevice, context_.surface, &capabilities) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get surface capabilities");
        }

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(context_.physicalDevice, context_.surface, &formatCount, nullptr);
        if (formatCount == 0) {
            throw std::runtime_error("No surface formats supported");
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(context_.physicalDevice, context_.surface, &formatCount, formats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(context_.physicalDevice, context_.surface, &presentModeCount, nullptr);
        if (presentModeCount == 0) {
            throw std::runtime_error("No present modes supported");
        }

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(context_.physicalDevice, context_.surface, &presentModeCount, presentModes.data());

        // Choose swapchain settings
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        VkExtent2D extent = chooseSwapExtent(capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        // Create swapchain
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = context_.surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {context_.graphicsQueueFamily, context_.presentQueueFamily};
        if (context_.graphicsQueueFamily != context_.presentQueueFamily) {
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

        if (vkCreateSwapchainKHR(context_.device, &createInfo, nullptr, &context_.swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain");
        }

        // Get swapchain images
        vkGetSwapchainImagesKHR(context_.device, context_.swapchain, &imageCount, nullptr);
        context_.swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(context_.device, context_.swapchain, &imageCount, context_.swapchainImages.data());

        context_.swapchainImageFormat = surfaceFormat.format;
        context_.swapchainExtent = extent;

        createImageViews();
    }

    void VulkanSwapchainManager::createImageViews() {
        SDL_Log("creating imageviews");
        context_.swapchainImageViews.resize(context_.swapchainImages.size());

        for (size_t i = 0; i < context_.swapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = context_.swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = context_.swapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(context_.device, &createInfo, nullptr, &context_.swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views");
            }
        }

        createRenderPass();
    }

    void VulkanSwapchainManager::createRenderPass() {
        SDL_Log("creating renderpass");
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = context_.swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(context_.device, &renderPassInfo, nullptr, &context_.renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass");
        }

        createFramebuffers();
    }

    void VulkanSwapchainManager::createFramebuffers() {
        SDL_Log("creating framebuffers");
        context_.swapchainFramebuffers.resize(context_.swapchainImageViews.size());

        for (size_t i = 0; i < context_.swapchainImageViews.size(); i++) {
            VkImageView attachments[] = {context_.swapchainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = context_.renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = context_.swapchainExtent.width;
            framebufferInfo.height = context_.swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(context_.device, &framebufferInfo, nullptr, &context_.swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer");
            }
        }
        createCommandPool();
    }

    void VulkanSwapchainManager::createCommandPool() {
        SDL_Log("creating command pools");
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = context_.graphicsQueueFamily;

        if (vkCreateCommandPool(context_.device, &poolInfo, nullptr, &context_.commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool");
        }

        createCommandBuffers();
    }

    void VulkanSwapchainManager::createCommandBuffers() {
        SDL_Log("creating command buffers");
        context_.commandBuffers.resize(context_.swapchainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = context_.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)context_.commandBuffers.size();

        if (vkAllocateCommandBuffers(context_.device, &allocInfo, context_.commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers");
        }

        createSyncObjects();
    }

    void VulkanSwapchainManager::createSyncObjects() {
        SDL_Log("Creating synchronization objects...");

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled

        VkResult result;

        result = vkCreateSemaphore(context_.device, &semaphoreInfo, nullptr, &context_.imageAvailableSemaphore);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image available semaphore");
        }

        result = vkCreateSemaphore(context_.device, &semaphoreInfo, nullptr, &context_.renderFinishedSemaphore);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render finished semaphore");
        }

        result = vkCreateFence(context_.device, &fenceInfo, nullptr, &context_.inFlightFence);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create in-flight fence");
        }

        SDL_Log("Created sync objects: fence=%p", (void*)context_.inFlightFence);
    }

    void VulkanSwapchainManager::cleanupSwapchain() {
        SDL_Log("cleaning up swapchains");
        for (auto framebuffer : context_.swapchainFramebuffers) {
            vkDestroyFramebuffer(context_.device, framebuffer, nullptr);
        }

        for (auto imageView : context_.swapchainImageViews) {
            vkDestroyImageView(context_.device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(context_.device, context_.swapchain, nullptr);
    }

    void VulkanSwapchainManager::recreateSwapchain() {
        SDL_Log("recreating swapchains");
        vkDeviceWaitIdle(context_.device);
        cleanupSwapchain();

        createSwapchain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createCommandBuffers();
    }

    VkSurfaceFormatKHR VulkanSwapchainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        SDL_Log("choosing swapsurface format");
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        SDL_Log("choosing swap present mode");
        //SDL_Log("Current SDL video driver: %s", SDL_GetCurrentVideoDriver());

        if (SDL_GetCurrentVideoDriver() && strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
            SDL_Log("Wayland detected, FIFO selected");
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        SDL_Log("choosing swap extent");
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            SDL_GetWindowSizeInPixels(m_window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
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
}
