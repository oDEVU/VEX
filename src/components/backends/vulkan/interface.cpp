#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "interface.hpp"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <set>

namespace vex {
    Interface::Interface(SDL_Window* window) {
        constexpr uint32_t apiVersion = VK_API_VERSION_1_3;

        // Initialize Volk with SDL's loader
        SDL_Log("Loading Vulkan library...");
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            throw std::runtime_error(SDL_GetError());
        }

        SDL_Log("Initializing Volk...");
        volkInitializeCustom(reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()));

        // Get required extensions
        SDL_Log("Creating Vulkan instance...");
        uint32_t sdlExtensionCount = 0;
        const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        std::vector<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

        // Add required instance extensions
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

        // Enable validation layers
        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        // Application info
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VEX Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "VEX";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = apiVersion;

        // Instance create info
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

#ifdef __APPLE__
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        // Create instance
        if (vkCreateInstance(&createInfo, nullptr, &context.instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance");
        }

        // Load Volk instance functions
        volkLoadInstance(context.instance);

        SDL_Log("Binding window...");

        if (!SDL_Vulkan_CreateSurface(window, context.instance, nullptr, &context.surface)) {
            throw std::runtime_error("Failed to create Vulkan surface: " + std::string(SDL_GetError()));
        }

        m_window = window;

        // Pick physical device
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(context.instance, &deviceCount, devices.data());

        // Select the first suitable device
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            SDL_Log("Selected GPU: %s", deviceProperties.deviceName);

            // Check queue families
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies) {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    context.graphicsQueueFamily = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &presentSupport);

                if (presentSupport) {
                    context.presentQueueFamily = i;
                }

                if (context.graphicsQueueFamily != UINT32_MAX && context.presentQueueFamily != UINT32_MAX) {
                    break;
                }
                i++;
            }

            if (context.graphicsQueueFamily != UINT32_MAX && context.presentQueueFamily != UINT32_MAX) {
                context.physicalDevice = device;
                break;
            }
        }

        if (context.physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU");
        }

        // Create logical device
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {context.graphicsQueueFamily, context.presentQueueFamily};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Device extensions
        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
#ifdef __APPLE__
            , VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
        };

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(context.physicalDevice, &deviceCreateInfo, nullptr, &context.device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device");
        }

        // Load Volk device functions
        volkLoadDevice(context.device);

        // Get queues
        vkGetDeviceQueue(context.device, context.graphicsQueueFamily, 0, &context.graphicsQueue);
        vkGetDeviceQueue(context.device, context.presentQueueFamily, 0, &context.presentQueue);

        // Initialize VMA
        VmaVulkanFunctions vmaFuncs = {};
        vmaFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vmaFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        vmaFuncs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        vmaFuncs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vmaFuncs.vkAllocateMemory = vkAllocateMemory;
        vmaFuncs.vkFreeMemory = vkFreeMemory;
        vmaFuncs.vkMapMemory = vkMapMemory;
        vmaFuncs.vkUnmapMemory = vkUnmapMemory;
        vmaFuncs.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        vmaFuncs.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
        vmaFuncs.vkBindBufferMemory = vkBindBufferMemory;
        vmaFuncs.vkBindImageMemory = vkBindImageMemory;
        vmaFuncs.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        vmaFuncs.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
        vmaFuncs.vkCreateBuffer = vkCreateBuffer;
        vmaFuncs.vkDestroyBuffer = vkDestroyBuffer;
        vmaFuncs.vkCreateImage = vkCreateImage;
        vmaFuncs.vkDestroyImage = vkDestroyImage;
        vmaFuncs.vkCmdCopyBuffer = vkCmdCopyBuffer;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = context.physicalDevice;
        allocatorInfo.device = context.device;
        allocatorInfo.instance = context.instance;
        allocatorInfo.vulkanApiVersion = apiVersion;
        allocatorInfo.pVulkanFunctions = &vmaFuncs;

        if (vmaCreateAllocator(&allocatorInfo, &context.allocator) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator");
        }

        // Initialize rendering resources
        SDL_Log("Initializing all Vulkan objects needed for rendering...");
        createSwapchain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();

        SDL_Log("Vulkan interface initialized successfully");
    }

    Interface::~Interface() {
        vkDeviceWaitIdle(context.device);

        cleanupSwapchain();

        // Destroy sync objects
        if (context.imageAvailableSemaphore) {
            vkDestroySemaphore(context.device, context.imageAvailableSemaphore, nullptr);
            context.imageAvailableSemaphore = VK_NULL_HANDLE;
        }
        if (context.renderFinishedSemaphore) {
            vkDestroySemaphore(context.device, context.renderFinishedSemaphore, nullptr);
            context.renderFinishedSemaphore = VK_NULL_HANDLE;
        }
        if (context.inFlightFence) {
            vkDestroyFence(context.device, context.inFlightFence, nullptr);
            context.inFlightFence = VK_NULL_HANDLE;
        }

        context.commandBuffers.clear();

        if (context.commandPool) {
            vkDestroyCommandPool(context.device, context.commandPool, nullptr);
            context.commandPool = VK_NULL_HANDLE;
        }

        if (context.renderPass) {
            vkDestroyRenderPass(context.device, context.renderPass, nullptr);
            context.renderPass = VK_NULL_HANDLE;
        }

        for (auto& framebuffer : context.swapchainFramebuffers) {
            if (framebuffer){
                vkDestroyFramebuffer(context.device, framebuffer, nullptr);
            }
        }
        context.swapchainFramebuffers.clear();

        // Destroy image views (if not already destroyed by cleanupSwapchain)
        for (auto& imageView : context.swapchainImageViews) {
            if (imageView) {
                vkDestroyImageView(context.device, imageView, nullptr);
            }
        }
        context.swapchainImageViews.clear();

        if (context.allocator) {
            vmaDestroyAllocator(context.allocator);
            context.allocator = nullptr;
        }

        if (context.device) {
            vkDestroyDevice(context.device, nullptr);
            context.device = VK_NULL_HANDLE;
        }

        if (context.surface) {
            vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
            context.surface = VK_NULL_HANDLE;
        }

        if (context.instance) {
            vkDestroyInstance(context.instance, nullptr);
            context.instance = VK_NULL_HANDLE;
        }

        SDL_Vulkan_UnloadLibrary();
    }

    void Interface::bindWindow(SDL_Window* window) {

        SDL_Log("Binding window...");

        if (context.surface) return;

        if (!SDL_Vulkan_CreateSurface(window, context.instance, nullptr, &context.surface)) {
            throw std::runtime_error("Failed to create Vulkan surface: " + std::string(SDL_GetError()));
        }

        m_window = window;

        // Initialize all Vulkan objects needed for rendering

        SDL_Log("Initializing all Vulkan objects needed for rendering...");
        createSwapchain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void Interface::unbindWindow() {
        if (!context.surface) return;

        vkDeviceWaitIdle(context.device);
        cleanupSwapchain();

        vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
        context.surface = VK_NULL_HANDLE;
    }

    void Interface::renderFrame() {
        // Wait for previous frame to finish
        VkResult result = vkWaitForFences(context.device, 1, &context.inFlightFence, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS) {
            SDL_Log("Failed to wait for fence: %d", result);
            throw std::runtime_error("Failed to wait for fence");
        }

        // Reset the fence before use
        result = vkResetFences(context.device, 1, &context.inFlightFence);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to reset fence");
        }

        // Acquire next image from swapchain
        uint32_t imageIndex;
        result = vkAcquireNextImageKHR(
            context.device,
            context.swapchain,
            UINT64_MAX,
            context.imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swapchain image");
        }

        // Reset command buffer
        vkResetCommandBuffer(context.commandBuffers[imageIndex], 0);

        // Record command buffer
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(context.commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = context.renderPass;
        renderPassInfo.framebuffer = context.swapchainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = context.swapchainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(context.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(context.commandBuffers[imageIndex]);

        if (vkEndCommandBuffer(context.commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer");
        }

        // Submit command buffer
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {context.imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &context.commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {context.renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, context.inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer");
        }

        // Present the frame
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = {context.swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swapchain image");
        }
    }

    // Helper methods implementation
    void Interface::createSwapchain() {
        SDL_Log("Creating Swapchain");
        // Query swapchain support
        VkSurfaceCapabilitiesKHR capabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &capabilities) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get surface capabilities");
        }

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, nullptr);
        if (formatCount == 0) {
            throw std::runtime_error("No surface formats supported");
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, formats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &presentModeCount, nullptr);
        if (presentModeCount == 0) {
            throw std::runtime_error("No present modes supported");
        }

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &presentModeCount, presentModes.data());

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
        createInfo.surface = context.surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {context.graphicsQueueFamily, context.presentQueueFamily};
        if (context.graphicsQueueFamily != context.presentQueueFamily) {
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

        if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain");
        }

        // Get swapchain images
        vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, nullptr);
        context.swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, context.swapchainImages.data());

        context.swapchainImageFormat = surfaceFormat.format;
        context.swapchainExtent = extent;
    }

    void Interface::createImageViews() {
        SDL_Log("creating imageviews");
        context.swapchainImageViews.resize(context.swapchainImages.size());

        for (size_t i = 0; i < context.swapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = context.swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = context.swapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(context.device, &createInfo, nullptr, &context.swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views");
            }
        }
    }

    void Interface::createRenderPass() {
        SDL_Log("creating renderpass");
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = context.swapchainImageFormat;
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

        if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &context.renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass");
        }
    }

    void Interface::createFramebuffers() {
        SDL_Log("creating framebuffers");
        context.swapchainFramebuffers.resize(context.swapchainImageViews.size());

        for (size_t i = 0; i < context.swapchainImageViews.size(); i++) {
            VkImageView attachments[] = {context.swapchainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = context.renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = context.swapchainExtent.width;
            framebufferInfo.height = context.swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &context.swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer");
            }
        }
    }

    void Interface::createCommandPool() {
        SDL_Log("creating command pools");
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = context.graphicsQueueFamily;

        if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &context.commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool");
        }
    }

    void Interface::createCommandBuffers() {
        SDL_Log("creating command buffers");
        context.commandBuffers.resize(context.swapchainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = context.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)context.commandBuffers.size();

        if (vkAllocateCommandBuffers(context.device, &allocInfo, context.commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers");
        }
    }

    void Interface::createSyncObjects() {
        SDL_Log("Creating synchronization objects...");

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled

        VkResult result;

        result = vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.imageAvailableSemaphore);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image available semaphore");
        }

        result = vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.renderFinishedSemaphore);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render finished semaphore");
        }

        result = vkCreateFence(context.device, &fenceInfo, nullptr, &context.inFlightFence);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create in-flight fence");
        }

        SDL_Log("Created sync objects: fence=%p", (void*)context.inFlightFence);
    }

    void Interface::cleanupSwapchain() {
        SDL_Log("cleaning up swapchains");
        for (auto framebuffer : context.swapchainFramebuffers) {
            vkDestroyFramebuffer(context.device, framebuffer, nullptr);
        }

        for (auto imageView : context.swapchainImageViews) {
            vkDestroyImageView(context.device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
    }

    void Interface::recreateSwapchain() {
        SDL_Log("recreating swapchains");
        vkDeviceWaitIdle(context.device);
        cleanupSwapchain();

        createSwapchain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createCommandBuffers();
    }

    VkSurfaceFormatKHR Interface::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        SDL_Log("choosing swapsurface format");
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR Interface::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        SDL_Log("choosing swap present mode");
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D Interface::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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
