#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "Interface.hpp"
#include <vector>
#include "components/errorUtils.hpp"
#include <algorithm>
#include <set>
#include <fstream>

namespace vex {
    Interface::Interface(SDL_Window* window, glm::uvec2 initialResolution, GameInfo gInfo, VirtualFileSystem* vfs) : m_p_window(window), m_vfs(vfs) {
        constexpr uint32_t apiVersion = VK_API_VERSION_1_3;

        m_context.currentRenderResolution = initialResolution;

        log("Loading Vulkan library...");
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            throw_error(SDL_GetError());
        }

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

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_context.instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw_error("Failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_context.instance, &deviceCount, devices.data());

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
        m_p_swapchainManager = std::make_unique<VulkanSwapchainManager>(m_context, m_p_window);
        m_p_swapchainManager->createSwapchain();

        log("Initializing Resources...");
        m_p_resources = std::make_unique<VulkanResources>(m_context);

        log("Initializing Mesh Manager...");
        m_p_meshManager = std::make_unique<MeshManager>(m_context, m_p_resources, m_vfs);

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

        log("Initializing Renderer...");
        m_p_renderer = std::make_unique<Renderer>(m_context, m_p_resources, m_p_pipeline, m_p_swapchainManager, m_p_meshManager);

        log("Vulkan interface initialized successfully");
    }

    Interface::~Interface() {
        vkDeviceWaitIdle(m_context.device);

        m_p_renderer.reset();
        m_p_meshManager.reset();
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
}
