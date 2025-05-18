#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "interface.hpp"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <set>

namespace vex {
    Interface::Interface(SDL_Window* window) : window_(window) {
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
        SDL_Log("Initializing Swapchain Manager...");

        swapchainManager_ = std::make_unique<VulkanSwapchainManager>(context,window_);
        swapchainManager_->createSwapchain();

        // Initialize rendering resources
        SDL_Log("Initializing Resources...");
        resources_ = std::make_unique<VulkanResources>(context);
        SDL_Log("Initializing Pipeline...");
        pipeline_ = std::make_unique<VulkanPipeline>(context);

        // Define vertex layout
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> attributes(3);
        attributes[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)};
        attributes[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
        attributes[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)};

        pipeline_->createGraphicsPipeline(
            "shaders/basic.vert.spv",
            "shaders/basic.frag.spv",
            bindingDesc,
            attributes
        );

        SDL_Log("Vulkan interface initialized successfully");
    }

    Interface::~Interface() {
        vkDeviceWaitIdle(context.device);

        swapchainManager_->cleanupSwapchain();


        // Cleanup synchronization objects
        for (size_t i = 0; i < context.MAX_FRAMES_IN_FLIGHT; i++) {
            if (context.imageAvailableSemaphores[i]) {
                vkDestroySemaphore(context.device, context.imageAvailableSemaphores[i], nullptr);
                context.imageAvailableSemaphores[i] = VK_NULL_HANDLE;
            }
            if (context.renderFinishedSemaphores[i]) {
                vkDestroySemaphore(context.device, context.renderFinishedSemaphores[i], nullptr);
                context.renderFinishedSemaphores[i] = VK_NULL_HANDLE;
            }
            if (context.inFlightFences[i]) {
                vkDestroyFence(context.device, context.inFlightFences[i], nullptr);
                context.inFlightFences[i] = VK_NULL_HANDLE;
            }
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

    Model& Interface::loadModel(const std::string& path, const std::string& name) {
        SDL_Log("Loading model: %s...", name);
        if (modelRegistry_.count(name)) {
            throw std::runtime_error("Model '" + name + "' already exists");
        }

        MeshData meshData;
        meshData.loadFromFile(path);

        models_.emplace_back();
        Model& model = models_.back();
        model.textureName = name;  // Set texture name to model name
        model.meshData = std::move(meshData);

        vulkanMeshes_.push_back(std::make_unique<VulkanMesh>(context));
        vulkanMeshes_.back()->upload(model.meshData);

        if (!model.meshData.texturePath.empty()) {
            SDL_Log("Loading texture...");
            resources_->loadTexture(model.meshData.texturePath, name);
        }

        modelRegistry_[name] = models_.size() - 1;
        return model;
    }

    void Interface::unloadModel(const std::string& name) {
        auto it = modelRegistry_.find(name);
        if (it == modelRegistry_.end()) return;

        const size_t index = it->second;

        // Remove GPU resources
        vulkanMeshes_[index].reset();
        models_.erase(models_.begin() + index);
        vulkanMeshes_.erase(vulkanMeshes_.begin() + index);

        // Update registry
        modelRegistry_.erase(name);
        for (auto& pair : modelRegistry_) {
            if (pair.second > index) pair.second--;
        }

        // Unload texture
        resources_->unloadTexture(name);
    }

    Model* Interface::getModel(const std::string& name) {
        auto it = modelRegistry_.find(name);
        return (it != modelRegistry_.end()) ? &models_[it->second] : nullptr;
    }
    void Interface::createDefaultTexture() {
        resources_->createDefaultTexture();
    }

    void Interface::bindWindow(SDL_Window* window) {

        SDL_Log("Binding window...");

        if (context.surface) return;

        if (!SDL_Vulkan_CreateSurface(window, context.instance, nullptr, &context.surface)) {
            throw std::runtime_error("Failed to create Vulkan surface: " + std::string(SDL_GetError()));
        }

        window_ = window;

        SDL_Log("Initializing Swapchain...");
        swapchainManager_->createSwapchain();
    }

    void Interface::unbindWindow() {
        if (!context.surface) return;

        vkDeviceWaitIdle(context.device);
        swapchainManager_->cleanupSwapchain();

        vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
        context.surface = VK_NULL_HANDLE;
    }

    void Interface::renderFrame(const glm::mat4& view, const glm::mat4& proj) {
        // Wait for previous frame
        vkWaitForFences(
            context.device,
            1,
            &context.inFlightFences[context.currentFrame],
            VK_TRUE,
            UINT64_MAX
        );

        // Acquire next image
        VkResult result = vkAcquireNextImageKHR(
            context.device,
            context.swapchain,
            UINT64_MAX,
            context.imageAvailableSemaphores[context.currentFrame],
            VK_NULL_HANDLE,
            &context.currentImageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            swapchainManager_->recreateSwapchain();
            return;
        }

        // Reset fence
        vkResetFences(context.device, 1, &context.inFlightFences[context.currentFrame]);

        // Update camera UBO
        resources_->updateCameraUBO({view, proj});

        // Bind default texture explicitly
        VkImageView textureView = resources_->getTextureView("default");
        resources_->updateTextureDescriptor(context.currentFrame, textureView);

        if (vulkanMeshes_.empty()) {

            // Record command buffer
            VkCommandBuffer commandBuffer = context.commandBuffers[context.currentImageIndex];
            vkResetCommandBuffer(commandBuffer, 0);

            // Still submit minimal command buffer
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(context.commandBuffers[context.currentImageIndex], &beginInfo);

            VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = context.renderPass;
                    renderPassInfo.framebuffer = context.swapchainFramebuffers[context.currentImageIndex];
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = context.swapchainExtent;

                    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = &clearColor;

                    vkCmdBeginRenderPass(context.commandBuffers[context.currentImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdEndRenderPass(context.commandBuffers[context.currentImageIndex]);

                    if (vkEndCommandBuffer(context.commandBuffers[context.currentImageIndex]) != VK_SUCCESS) {
                        throw std::runtime_error("Failed to record command buffer");
                    }

                    // Submit command buffer
                    VkSubmitInfo submitInfo = {};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                    VkSemaphore waitSemaphores[] = {context.imageAvailableSemaphores[context.currentFrame]};
                    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                    submitInfo.waitSemaphoreCount = 1;
                    submitInfo.pWaitSemaphores = waitSemaphores;
                    submitInfo.pWaitDstStageMask = waitStages;
                    submitInfo.commandBufferCount = 1;
                    submitInfo.pCommandBuffers = &context.commandBuffers[context.currentImageIndex];

                    VkSemaphore signalSemaphores[] = {context.renderFinishedSemaphores[context.currentFrame]};
                    submitInfo.signalSemaphoreCount = 1;
                    submitInfo.pSignalSemaphores = signalSemaphores;

                    if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, context.inFlightFences[context.currentFrame]) != VK_SUCCESS) {
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
                    presentInfo.pImageIndices = &context.currentImageIndex;
                    presentInfo.pResults = nullptr;

                    result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

                    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                        swapchainManager_->recreateSwapchain();
                    } else if (result != VK_SUCCESS) {
                        throw std::runtime_error("Failed to present swapchain image");
                    }
            return;
        }

        // Record command buffer
        VkCommandBuffer commandBuffer = context.commandBuffers[context.currentImageIndex];
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // Render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = context.renderPass;
        renderPassInfo.framebuffer = context.swapchainFramebuffers[context.currentImageIndex];
        renderPassInfo.renderArea.extent = context.swapchainExtent;

        VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Draw models
        for (size_t i = 0; i < models_.size(); i++) {

            std::string texName;

            // First try model's texture
            if (!models_[i].textureName.empty() &&
                resources_->textureExists(models_[i].textureName))
            {
                texName = models_[i].textureName;
            }
            // Fallback to default
            else {
                texName = resources_->getDefaultTextureName();
            }

            // Get texture for this model
            VkImageView textureView = resources_->getTextureView(texName);
            resources_->updateTextureDescriptor(context.currentFrame, textureView);

            // Update model UBO
            resources_->updateModelUBO(context.currentFrame,
                                     {models_[i].transform.matrix()});

            // Bind pipeline and descriptors
            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_->get()
            );

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_->layout(),
                0, 1,
                resources_->getDescriptorSetPtr(context.currentFrame), // Use pointer getter
                0, nullptr
            );

            // Draw
            vulkanMeshes_[i]->draw(commandBuffer);
        }

        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        // Submit
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {context.imageAvailableSemaphores[context.currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {context.renderFinishedSemaphores[context.currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkQueueSubmit(
            context.graphicsQueue,
            1,
            &submitInfo,
            context.inFlightFences[context.currentFrame]
        );

        // Present
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = {context.swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &context.currentImageIndex;

        result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            swapchainManager_->recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        context.currentFrame = (context.currentFrame + 1) % context.MAX_FRAMES_IN_FLIGHT;
    }
}
