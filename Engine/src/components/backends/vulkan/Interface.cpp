#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "Interface.hpp"
#include <vector>

#include "components/errorUtils.hpp"

#include <algorithm>
#include <set>
#include <fstream>
#include <unordered_set>

namespace vex {
    Interface::Interface(SDL_Window* window, glm::uvec2 initialResolution, GameInfo gInfo) : m_p_window(window){
        constexpr uint32_t apiVersion = VK_API_VERSION_1_3;

        m_context.currentRenderResolution = initialResolution;

        log("Loading Vulkan library...");
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            throw_error(SDL_GetError());
        }

        // Init stuff

        log("Initializing Volk...");
        volkInitializeCustom(reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()));

        log("Creating Vulkan instance...");
        uint32_t sdlExtensionCount = 0;
        const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        std::vector<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

#if DEBUG
        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
#else
        const std::vector<const char*> validationLayers;
#endif

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = gInfo.projectName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(gInfo.versionMajor, gInfo.versionMinor, gInfo.versionPatch);
        appInfo.pEngineName = "VEX";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = apiVersion;

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

        if (vkCreateInstance(&createInfo, nullptr, &m_context.instance) != VK_SUCCESS) {
            throw_error("Failed to create Vulkan instance");
        }

        volkLoadInstance(m_context.instance);

        log("Binding window...");
        if (!SDL_Vulkan_CreateSurface(window, m_context.instance, nullptr, &m_context.surface)) {
            throw_error("Failed to create Vulkan surface: " + std::string(SDL_GetError()));
        }

        // Device stuff
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_context.instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw_error("Failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_context.instance, &deviceCount, devices.data());

        // Select the first suitable device, FIX ME!
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            log("Selected GPU: %s", deviceProperties.deviceName);

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies) {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    m_context.graphicsQueueFamily = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_context.surface, &presentSupport);

                if (presentSupport) {
                    m_context.presentQueueFamily = i;
                }

                if (m_context.graphicsQueueFamily != UINT32_MAX && m_context.presentQueueFamily != UINT32_MAX) {
                    break;
                }
                i++;
            }

            if (m_context.graphicsQueueFamily != UINT32_MAX && m_context.presentQueueFamily != UINT32_MAX) {
                m_context.physicalDevice = device;
                break;
            }
        }

        if (m_context.physicalDevice == VK_NULL_HANDLE) {
            throw_error("Failed to find a suitable GPU");
        }

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {m_context.graphicsQueueFamily, m_context.presentQueueFamily};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Device extensions, They are fucking broken :C
        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
#ifdef __APPLE__
            , VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
        };

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
        dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamicRenderingFeature.dynamicRendering = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = &dynamicRenderingFeature;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_context.physicalDevice, &deviceCreateInfo, nullptr, &m_context.device) != VK_SUCCESS) {
            throw_error("Failed to create logical device");
        }

        volkLoadDevice(m_context.device);

        vkGetDeviceQueue(m_context.device, m_context.graphicsQueueFamily, 0, &m_context.graphicsQueue);
        vkGetDeviceQueue(m_context.device, m_context.presentQueueFamily, 0, &m_context.presentQueue);

        // VMA STUFF
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
        allocatorInfo.physicalDevice = m_context.physicalDevice;
        allocatorInfo.device = m_context.device;
        allocatorInfo.instance = m_context.instance;
        allocatorInfo.vulkanApiVersion = apiVersion;
        allocatorInfo.pVulkanFunctions = &vmaFuncs;

        if (vmaCreateAllocator(&allocatorInfo, &m_context.allocator) != VK_SUCCESS) {
            throw_error("Failed to create VMA allocator");
        }

        log("Initializing Swapchain Manager...");

        m_p_swapchainManager = std::make_unique<VulkanSwapchainManager>(m_context,m_p_window);
        m_p_swapchainManager->createSwapchain();

        log("Initializing Resources...");
        m_p_resources = std::make_unique<VulkanResources>(m_context);
        log("Initializing Pipeline...");
        m_p_pipeline = std::make_unique<VulkanPipeline>(m_context);

        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> attributes(3);
        attributes[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)};
        attributes[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
        attributes[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)};

        m_p_pipeline->createGraphicsPipeline(
            "Engine/shaders/basic.vert.spv",
            "Engine/shaders/basic.frag.spv",
            bindingDesc,
            attributes
        );

        startTime = std::chrono::high_resolution_clock::now();
        log("Vulkan interface initialized successfully");
    }

    Interface::~Interface() {
        vkDeviceWaitIdle(m_context.device);

        m_modelRegistry.clear();
        m_models.clear();
        m_vulkanMeshes.clear();

        m_p_resources.reset();

        m_p_pipeline.reset();

        m_p_swapchainManager->cleanupSwapchain();
        m_p_swapchainManager.reset();

        if (m_context.textureDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_context.device, m_context.textureDescriptorSetLayout, nullptr);
            m_context.textureDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_context.uboDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_context.device, m_context.uboDescriptorSetLayout, nullptr);
            m_context.uboDescriptorSetLayout = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < m_context.MAX_FRAMES_IN_FLIGHT; i++) {
            if (m_context.imageAvailableSemaphores[i]) {
                vkDestroySemaphore(m_context.device, m_context.imageAvailableSemaphores[i], nullptr);
                m_context.imageAvailableSemaphores[i] = VK_NULL_HANDLE;
            }
            if (m_context.renderFinishedSemaphores[i]) {
                vkDestroySemaphore(m_context.device, m_context.renderFinishedSemaphores[i], nullptr);
                m_context.renderFinishedSemaphores[i] = VK_NULL_HANDLE;
            }
            if (m_context.inFlightFences[i]) {
                vkDestroyFence(m_context.device, m_context.inFlightFences[i], nullptr);
                m_context.inFlightFences[i] = VK_NULL_HANDLE;
            }
        }

        m_context.commandBuffers.clear();

        if (m_context.commandPool) {
            vkDestroyCommandPool(m_context.device, m_context.commandPool, nullptr);
            m_context.commandPool = VK_NULL_HANDLE;
        }

        for (auto& imageView : m_context.swapchainImageViews) {
            if (imageView) {
                vkDestroyImageView(m_context.device, imageView, nullptr);
            }
        }
        m_context.swapchainImageViews.clear();

        if (m_context.allocator) {
            vmaDestroyAllocator(m_context.allocator);
            m_context.allocator = nullptr;
        }

        if (m_context.device) {
            vkDestroyDevice(m_context.device, nullptr);
            m_context.device = VK_NULL_HANDLE;
        }

        if (m_context.surface) {
            vkDestroySurfaceKHR(m_context.instance, m_context.surface, nullptr);
            m_context.surface = VK_NULL_HANDLE;
        }

        if (m_context.instance) {
            vkDestroyInstance(m_context.instance, nullptr);
            m_context.instance = VK_NULL_HANDLE;
        }

        SDL_Vulkan_UnloadLibrary();
    }


    Model& Interface::loadModel(const std::string& path, const std::string& name) {
        log("Loading model: %s...", name.c_str());
        if (m_modelRegistry.count(name)) {
            throw_error("Model '" + name + "' already exists");
        }

        MeshData meshData;
        try {
            log("Loading mesh data from: %s", path.c_str());
            std::ifstream fileCheck(path);
            if (!fileCheck.is_open()) {
                throw_error("File not found: " + path);
            }
            fileCheck.close();
            meshData.loadFromFile(path);
        } catch (const std::exception& e) {
            SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Model load failed");
            handle_exception(e);
        }

            uint32_t newId;
            if (!m_freeModelIds.empty()) {
                newId = m_freeModelIds.back();
                m_freeModelIds.pop_back();
            } else {
                newId = m_nextModelId++;
                if (newId >= m_context.MAX_MODELS) {
                    throw_error("Maximum model count exceeded");
                }
            }

        m_models.emplace_back(std::make_unique<Model>());
        Model& model = *m_models.back();
        model.id = newId;
        model.meshData = std::move(meshData);

        std::unordered_set<std::string> uniqueTextures;
        for (const auto& submesh : model.meshData.submeshes) {
            if (!submesh.texturePath.empty()) {
                uniqueTextures.insert(submesh.texturePath);
            }
        }

        log("Loading %zu submesh textures", uniqueTextures.size());
        for (const auto& texPath : uniqueTextures) {
            log("Processing texture: %s", texPath.c_str());
            if (!m_p_resources->textureExists(texPath)) {
                try {
                    m_p_resources->loadTexture(texPath, texPath);
                    log("Loaded texture: %s", texPath.c_str());
                } catch (const std::exception& e) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                                "Failed to load texture %s",
                                texPath.c_str());
                    handle_exception(e);
                }
            } else {
                log("Texture already exists: %s", texPath.c_str());
            }
        }

        try {
            log("Creating Vulkan mesh for %s", name.c_str());
            m_vulkanMeshes.push_back(std::make_unique<VulkanMesh>(m_context));
            m_vulkanMeshes.back()->upload(model.meshData);
            log("Mesh upload successful");
        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Mesh upload failed");
            m_vulkanMeshes.pop_back();
            handle_exception(e);
        }

        m_modelRegistry[name] = &model;
        log("Model %s registered successfully", name.c_str());
        return model;
    }

    void Interface::unloadModel(const std::string& name) {

        auto it = m_modelRegistry.find(name);
        if (it == m_modelRegistry.end()) return;

            m_freeModelIds.push_back(it->second->id);

            auto modelIter = std::find_if(
                m_models.begin(), m_models.end(),
                [&](const auto& m) { return m.get() == it->second; }
            );
            if (modelIter != m_models.end()) m_models.erase(modelIter);

        m_modelRegistry.erase(name);
        m_p_resources->unloadTexture(name);
    }

    Model* Interface::getModel(const std::string& name) {
        auto it = m_modelRegistry.find(name);
        return (it != m_modelRegistry.end()) ? it->second : nullptr;
    }
    void Interface::createDefaultTexture() {
        m_p_resources->createDefaultTexture();
    }

    void Interface::bindWindow(SDL_Window *m_p_window) {

        log("Binding window...");

        if (m_context.surface) return;

        if (!SDL_Vulkan_CreateSurface(m_p_window, m_context.instance, nullptr, &m_context.surface)) {
            throw_error("Failed to create Vulkan surface: " + std::string(SDL_GetError()));
        }

        this->m_p_window = m_p_window;

        log("Initializing Swapchain...");
        m_p_swapchainManager->createSwapchain();
    }

    void Interface::unbindWindow() {
        if (!m_context.surface) return;

        vkDeviceWaitIdle(m_context.device);
        m_p_swapchainManager->cleanupSwapchain();

        vkDestroySurfaceKHR(m_context.instance, m_context.surface, nullptr);
        m_context.surface = VK_NULL_HANDLE;
    }

    void Interface::setRenderResolution(glm::uvec2 resolution) {
        m_context.currentRenderResolution = resolution;
        m_p_swapchainManager->recreateSwapchain();
        m_p_pipeline->updateViewport(resolution);
    }

    void Interface::transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                        VkImageLayout oldLayout, VkImageLayout newLayout,
                                        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(
            cmd,
            srcStage,
            dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    void Interface::renderFrame(const glm::mat4& view, const glm::mat4& proj, glm::uvec2 renderResolution, ImGUIWrapper& m_ui, u_int64_t frame) {
        if (renderResolution != m_context.currentRenderResolution) {
            m_context.currentRenderResolution = renderResolution;
            m_p_pipeline->updateViewport(renderResolution);
        }
        vkWaitForFences(
            m_context.device,
            1,
            &m_context.inFlightFences[m_context.currentFrame],
            VK_TRUE,
            UINT64_MAX
        );

        VkResult result = vkAcquireNextImageKHR(
            m_context.device,
            m_context.swapchain,
            UINT64_MAX,
            m_context.imageAvailableSemaphores[m_context.currentFrame],
            VK_NULL_HANDLE,
            &m_context.currentImageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            m_p_swapchainManager->recreateSwapchain();
            return;
        }

        vkResetFences(m_context.device, 1, &m_context.inFlightFences[m_context.currentFrame]);

        vkDeviceWaitIdle(m_context.device);

        m_p_resources->updateCameraUBO({view, proj});

        VkImageView textureView = m_p_resources->getTextureView("default");
        m_p_resources->updateTextureDescriptor(m_context.currentFrame, textureView, 0);

        VkCommandBuffer commandBuffer = m_context.commandBuffers[m_context.currentImageIndex];
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        transitionImageLayout(commandBuffer,
                            m_context.swapchainImages[m_context.currentImageIndex],
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            0,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT);

        transitionImageLayout(commandBuffer,
                            m_context.lowResColorImage,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            0,
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

           VkRenderingInfo renderingInfo{};
           renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
           renderingInfo.renderArea.offset = {0, 0};
           renderingInfo.renderArea.extent = {renderResolution.x, renderResolution.y};
           renderingInfo.layerCount = 1;

           VkRenderingAttachmentInfo colorAttachment{};
           colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
           colorAttachment.imageView = m_context.lowResColorView;
           colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
           colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
           colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
           colorAttachment.clearValue.color = {{0.1f, 0.1f, 0.1f, 1.0f}};

           VkRenderingAttachmentInfo depthAttachment{};
           depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
           depthAttachment.imageView = m_context.depthImageView;
           depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
           depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
           depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
           depthAttachment.clearValue.depthStencil = {1.0f, 0};

           renderingInfo.colorAttachmentCount = 1;
           renderingInfo.pColorAttachments = &colorAttachment;
           renderingInfo.pDepthAttachment = &depthAttachment;

           vkCmdBeginRendering(commandBuffer, &renderingInfo);

                  VkViewport viewport{};
                  viewport.x = 0.0f;
                  viewport.y = 0.0f;
                  viewport.width = static_cast<float>(m_context.swapchainExtent.width);
                  viewport.height = static_cast<float>(m_context.swapchainExtent.height);
                  viewport.minDepth = 0.0f;
                  viewport.maxDepth = 1.0f;
                  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                  VkRect2D scissor{};
                  scissor.offset = {0, 0};
                  scissor.extent = {m_context.swapchainExtent.width, m_context.swapchainExtent.height};
                  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkImageView defaultTexture = m_p_resources->getTextureView("default");
        m_p_resources->updateTextureDescriptor(m_context.currentFrame, defaultTexture, 0);

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline->get());


        auto now = std::chrono::high_resolution_clock::now();
        currentTime = std::chrono::duration<float>(now - startTime).count();

        for (auto& modelPtr : m_models) {
            auto& model = *modelPtr;
            auto& vulkanMesh = m_vulkanMeshes[model.id];

            // Update UBOs
            m_p_resources->updateCameraUBO({view, proj});
            m_p_resources->updateModelUBO(m_context.currentFrame, model.id, ModelUBO{model.transform.matrix()});
            //log("Updated UBOs for model %zu", i);
            vulkanMesh->draw(commandBuffer, m_p_pipeline->layout(), *m_p_resources, m_context.currentFrame, model.id, currentTime, m_context.currentRenderResolution);
        }

        if(frame != 0){
            m_ui.beginFrame();
            m_ui.executeUIFunctions();
            m_ui.endFrame();
        }

        vkCmdEndRendering(commandBuffer);

        transitionImageLayout(commandBuffer, m_context.lowResColorImage,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_ACCESS_TRANSFER_READ_BIT,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {(int)renderResolution.x, (int)renderResolution.y, 1};

        blit.dstSubresource = blit.srcSubresource;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {(int)m_context.swapchainExtent.width, (int)m_context.swapchainExtent.height, 1};

        vkCmdBlitImage(
            commandBuffer,
            m_context.lowResColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_context.swapchainImages[m_context.currentImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_NEAREST
        );

        VkImageMemoryBarrier presentBarrier{};
        presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentBarrier.image = m_context.swapchainImages[m_context.currentImageIndex];
        presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        presentBarrier.subresourceRange.levelCount = 1;
        presentBarrier.subresourceRange.layerCount = 1;
        presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentBarrier.dstAccessMask = 0;

        VkImageMemoryBarrier lowResRestoreBarrier{};
        lowResRestoreBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        lowResRestoreBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        lowResRestoreBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        lowResRestoreBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lowResRestoreBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lowResRestoreBarrier.image = m_context.lowResColorImage;
        lowResRestoreBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        lowResRestoreBarrier.subresourceRange.levelCount = 1;
        lowResRestoreBarrier.subresourceRange.layerCount = 1;
        lowResRestoreBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        lowResRestoreBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            2, (VkImageMemoryBarrier[]){presentBarrier, lowResRestoreBarrier}
        );

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_context.imageAvailableSemaphores[m_context.currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {m_context.renderFinishedSemaphores[m_context.currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkQueueSubmit(
            m_context.graphicsQueue,
            1,
            &submitInfo,
            m_context.inFlightFences[m_context.currentFrame]
        );

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = {m_context.swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &m_context.currentImageIndex;

        result = vkQueuePresentKHR(m_context.presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            m_p_swapchainManager->recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw_error("Failed to present swap chain image!");
        }

        m_context.currentFrame = (m_context.currentFrame + 1) % m_context.MAX_FRAMES_IN_FLIGHT;
    }
}
