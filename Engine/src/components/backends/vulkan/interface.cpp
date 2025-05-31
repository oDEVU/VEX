#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "interface.hpp"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <fstream>
#include <unordered_set>

namespace vex {
    Interface::Interface(SDL_Window* window, glm::uvec2 initialResolution) : window_(window){
        constexpr uint32_t apiVersion = VK_API_VERSION_1_3;

         context.currentRenderResolution = initialResolution;

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
        deviceFeatures.samplerAnisotropy = VK_TRUE; // Add this line

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
            "Engine/shaders/basic.vert.spv",
            "Engine/shaders/basic.frag.spv",
            bindingDesc,
            attributes
        );

        //setObjectName(context.device, (uint64_t)context.depthImage,
        //             VK_OBJECT_TYPE_IMAGE, "Depth Buffer");

        startTime = std::chrono::high_resolution_clock::now();
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
        SDL_Log("Loading model: %s...", name.c_str());
        if (modelRegistry_.count(name)) {
            throw std::runtime_error("Model '" + name + "' already exists");
        }

        MeshData meshData;
        try {
            SDL_Log("Loading mesh data from: %s", path.c_str());
            std::ifstream fileCheck(path);
            if (!fileCheck.is_open()) {
                throw std::runtime_error("File not found: " + path);
            }
            fileCheck.close();
            meshData.loadFromFile(path);
        } catch (const std::exception& e) {
            SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Model load failed: %s", e.what());
            throw;
        }

            // Assign a stable ID
            uint32_t newId;
            if (!freeModelIds_.empty()) {
                newId = freeModelIds_.back();
                freeModelIds_.pop_back();
            } else {
                newId = nextModelId_++;
                if (newId >= context.MAX_MODELS) {
                    throw std::runtime_error("Maximum model count exceeded");
                }
            }

        // Create new model entry
        models_.emplace_back(std::make_unique<Model>());
        Model& model = *models_.back();
        model.id = newId;
        model.meshData = std::move(meshData);

        // Collect unique texture paths from all submeshes
        std::unordered_set<std::string> uniqueTextures;
        for (const auto& submesh : model.meshData.submeshes) {
            if (!submesh.texturePath.empty()) {
                uniqueTextures.insert(submesh.texturePath);
            }
        }

        // Load all required textures
        SDL_Log("Loading %zu submesh textures", uniqueTextures.size());
        for (const auto& texPath : uniqueTextures) {
            SDL_Log("Processing texture: %s", texPath.c_str());
            if (!resources_->textureExists(texPath)) {
                try {
                    resources_->loadTexture(texPath, texPath);
                    SDL_Log("Loaded texture: %s", texPath.c_str());
                } catch (const std::exception& e) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                                "Failed to load texture %s: %s",
                                texPath.c_str(), e.what());
                    throw;
                }
            } else {
                SDL_Log("Texture already exists: %s", texPath.c_str());
            }
        }

        // Upload mesh to GPU
        try {
            SDL_Log("Creating Vulkan mesh for %s", name.c_str());
            vulkanMeshes_.push_back(std::make_unique<VulkanMesh>(context));
            vulkanMeshes_.back()->upload(model.meshData);
            SDL_Log("Mesh upload successful");
        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Mesh upload failed: %s", e.what());
            vulkanMeshes_.pop_back();
            throw;
        }

        // Register model
        modelRegistry_[name] = &model;
        SDL_Log("Model %s registered successfully", name.c_str());
        return model;
    }

    void Interface::unloadModel(const std::string& name) {

        auto it = modelRegistry_.find(name);
        if (it == modelRegistry_.end()) return;

            // Reclaim the ID
            freeModelIds_.push_back(it->second->id);

        // Find the model in the deque

            // Erase from deque
            auto modelIter = std::find_if(
                models_.begin(), models_.end(),
                [&](const auto& m) { return m.get() == it->second; }
            );
            if (modelIter != models_.end()) models_.erase(modelIter);

        // Remove from registry
        modelRegistry_.erase(name);

        // Unload texture
        resources_->unloadTexture(name);
    }

    Model* Interface::getModel(const std::string& name) {
        auto it = modelRegistry_.find(name);
        return (it != modelRegistry_.end()) ? it->second : nullptr;
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

    void Interface::setRenderResolution(glm::uvec2 resolution) {
        context.currentRenderResolution = resolution;
        // Recreate swapchain with new resolution
        swapchainManager_->recreateSwapchain();

        // Update viewport and scissor in pipeline
        pipeline_->updateViewport(resolution);
    }

    void Interface::renderFrame(const glm::mat4& view, const glm::mat4& proj, glm::uvec2 renderResolution) {
        if (renderResolution != context.currentRenderResolution) {
            context.currentRenderResolution = renderResolution;
            pipeline_->updateViewport(renderResolution);
        }
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
        resources_->updateTextureDescriptor(context.currentFrame, textureView, 0);


        if (!context.lowResFramebuffer || !context.lowResColorImage) {
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Low-res resources not ready - skipping frame");
            return;
        }


        // Record command buffer
        VkCommandBuffer commandBuffer = context.commandBuffers[context.currentImageIndex];
           vkResetCommandBuffer(commandBuffer, 0);

           VkCommandBufferBeginInfo beginInfo{};
           beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
           vkBeginCommandBuffer(commandBuffer, &beginInfo);

           // First render pass - render to low-res framebuffer
           VkRenderPassBeginInfo renderPassInfo{};
           renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
           renderPassInfo.renderPass = context.lowResRenderPass;
           renderPassInfo.framebuffer = context.lowResFramebuffer;
           renderPassInfo.renderArea.offset = {0, 0};
           renderPassInfo.renderArea.extent = {renderResolution.x, renderResolution.y};

               VkClearValue clearValues[2];
               clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
               clearValues[1].depthStencil = {1.0f, 0};
               renderPassInfo.clearValueCount = 2;
               renderPassInfo.pClearValues = clearValues;

               vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                  // Set viewport and scissor for low-res rendering
                  VkViewport viewport{};
                  viewport.x = 0.0f;
                  viewport.y = 0.0f;
                  viewport.width = static_cast<float>(context.swapchainExtent.width);
                  viewport.height = static_cast<float>(context.swapchainExtent.height);
                  viewport.minDepth = 0.0f;
                  viewport.maxDepth = 1.0f;
                  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                  VkRect2D scissor{};
                  scissor.offset = {0, 0};
                  scissor.extent = {context.swapchainExtent.width, context.swapchainExtent.height};
                  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Bind default texture initially
        VkImageView defaultTexture = resources_->getTextureView("default");
        resources_->updateTextureDescriptor(context.currentFrame, defaultTexture, 0);
        //SDL_Log("Bound default texture");



                    // Bind pipeline
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->get());


        auto now = std::chrono::high_resolution_clock::now();
        currentTime = std::chrono::duration<float>(now - startTime).count();

        for (auto& modelPtr : models_) {
            auto& model = *modelPtr;
            auto& vulkanMesh = vulkanMeshes_[model.id];

            // Update UBOs
            resources_->updateCameraUBO({view, proj});
            resources_->updateModelUBO(context.currentFrame, model.id, ModelUBO{model.transform.matrix()});
            //SDL_Log("Updated UBOs for model %zu", i);
            // Draw all submeshes
            vulkanMesh->draw(commandBuffer, pipeline_->layout(), *resources_, context.currentFrame, model.id, currentTime, context.currentRenderResolution);
        }


        vkCmdEndRenderPass(commandBuffer);

        // Transition low-res image for transfer
        VkImageMemoryBarrier lowResBarrier{};
        lowResBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        lowResBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        lowResBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        lowResBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lowResBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lowResBarrier.image = context.lowResColorImage;
        lowResBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        lowResBarrier.subresourceRange.levelCount = 1;
        lowResBarrier.subresourceRange.layerCount = 1;
        lowResBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        lowResBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        VkImageMemoryBarrier swapchainBarrier{};
        swapchainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swapchainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.image = context.swapchainImages[context.currentImageIndex];
        swapchainBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        swapchainBarrier.subresourceRange.levelCount = 1;
        swapchainBarrier.subresourceRange.layerCount = 1;
        swapchainBarrier.srcAccessMask = 0;
        swapchainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // Issue both barriers together
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // Source stage
            VK_PIPELINE_STAGE_TRANSFER_BIT,                 // Destination stage
            0,
            0, nullptr,
            0, nullptr,
            2, (VkImageMemoryBarrier[]){lowResBarrier, swapchainBarrier}
        );

        // Perform the blit with nearest-neighbor filtering
        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {(int)renderResolution.x, (int)renderResolution.y, 1};

        blit.dstSubresource = blit.srcSubresource;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {(int)context.swapchainExtent.width, (int)context.swapchainExtent.height, 1};

        vkCmdBlitImage(
            commandBuffer,
            context.lowResColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            context.swapchainImages[context.currentImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_NEAREST // Critical for pixel-perfect upscaling
        );

        // Transition swapchain image for presentation
        VkImageMemoryBarrier presentBarrier{};
        presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentBarrier.image = context.swapchainImages[context.currentImageIndex];
        presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        presentBarrier.subresourceRange.levelCount = 1;
        presentBarrier.subresourceRange.layerCount = 1;
        presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentBarrier.dstAccessMask = 0;

        // Transition low-res image back for rendering
        VkImageMemoryBarrier lowResRestoreBarrier{};
        lowResRestoreBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        lowResRestoreBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        lowResRestoreBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        lowResRestoreBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lowResRestoreBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lowResRestoreBarrier.image = context.lowResColorImage;
        lowResRestoreBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        lowResRestoreBarrier.subresourceRange.levelCount = 1;
        lowResRestoreBarrier.subresourceRange.layerCount = 1;
        lowResRestoreBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        lowResRestoreBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,                 // Source stage
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,   // Destination stage
            0,
            0, nullptr,
            0, nullptr,
            2, (VkImageMemoryBarrier[]){presentBarrier, lowResRestoreBarrier}
        );

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
