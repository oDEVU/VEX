//#include <vulkan/vulkan_core.h>
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "Interface.hpp"
#include <vector>
#include "components/errorUtils.hpp"
#include <algorithm>
#include <set>
#include <fstream>

#include <immintrin.h>
#include "../../HardwareInfo.hpp"

namespace vex {
    Interface::Interface(SDL_Window* window, glm::uvec2 initialResolution, GameInfo gInfo, VirtualFileSystem* vfs) : m_p_window(window), m_vfs(vfs) {
        constexpr uint32_t apiVersion = VK_API_VERSION_1_3;

        m_context.currentRenderResolution = initialResolution;

        try {

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

        VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
        int bestScore = -1;

        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            log("Avaiable GPU (%s): %s", deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "DISCRETE" : "INTEGRATED",  deviceProperties.deviceName);
            int score = 0;

            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                score += 1000;
            }

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
            uint32_t graphicsIdx = UINT32_MAX;
            uint32_t presentIdx = UINT32_MAX;

            int i = 0;
            for (const auto& queueFamily : queueFamilies) {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphicsIdx = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_context.surface, &presentSupport);

                if (presentSupport) {
                    presentIdx = i;
                }

                if (graphicsIdx != UINT32_MAX && presentIdx != UINT32_MAX)
                {
                    break;
                }
                i++;
            }

            if (graphicsIdx != UINT32_MAX && presentIdx != UINT32_MAX)
            {
                if (score > bestScore)
                {
                    bestScore = score;
                    selectedDevice = device;
                    m_context.graphicsQueueFamily = graphicsIdx;
                    m_context.presentQueueFamily = presentIdx;
                }
            }
        }

        if (selectedDevice != VK_NULL_HANDLE)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(selectedDevice, &deviceProperties);

            log("Selected GPU: %s", deviceProperties.deviceName);

            m_context.physicalDevice = selectedDevice;
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
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME
#ifdef __APPLE__
            , VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
        };

        VkPhysicalDeviceFeatures2 deviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceVulkan11Features features11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
        VkPhysicalDeviceMultiDrawFeaturesEXT multiDrawFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT };
        VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT };

        deviceFeatures2.pNext = &features11;
        features11.pNext = &features12;
        features12.pNext = &dynamicRenderingFeatures;
        dynamicRenderingFeatures.pNext = &multiDrawFeatures;
        multiDrawFeatures.pNext = &extendedDynamicState2Features;
        extendedDynamicState2Features.pNext = nullptr;

        vkGetPhysicalDeviceFeatures2(m_context.physicalDevice, &deviceFeatures2);

        if (deviceFeatures2.features.samplerAnisotropy) {
            deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
        } else {
            log(LogLevel::WARNING, "Sampler Anisotropy not supported.");
        }

        if (deviceFeatures2.features.multiDrawIndirect) {
            m_context.supportsIndirectDraw = true;
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(m_context.physicalDevice, &properties);
            m_context.maxDrawIndirectCount = properties.limits.maxDrawIndirectCount;
        } else {
            m_context.supportsIndirectDraw = false;
            deviceFeatures2.features.multiDrawIndirect = VK_FALSE;
        }

        if (features11.shaderDrawParameters) {
            features11.shaderDrawParameters = VK_TRUE;
            m_context.supportsShaderDrawParameters = true;
        } else {
            features11.shaderDrawParameters = VK_FALSE;
            m_context.supportsShaderDrawParameters = false;
        }

        if (features12.descriptorBindingPartiallyBound &&
            features12.runtimeDescriptorArray &&
            features12.shaderSampledImageArrayNonUniformIndexing &&
            features12.descriptorBindingSampledImageUpdateAfterBind) {

            m_context.supportsBindlessTextures = true;

            features12.descriptorBindingPartiallyBound = VK_TRUE;
            features12.runtimeDescriptorArray = VK_TRUE;
            features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        } else {
            m_context.supportsBindlessTextures = false;

            features12.descriptorBindingPartiallyBound = VK_FALSE;
            features12.runtimeDescriptorArray = VK_FALSE;
            features12.shaderSampledImageArrayNonUniformIndexing = VK_FALSE;
            log(LogLevel::WARNING, "Bindless textures not supported.");
        }

        if (dynamicRenderingFeatures.dynamicRendering) {
            dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
        } else {
            throw_error("Dynamic Rendering not supported by GPU!");
        }

        if (multiDrawFeatures.multiDraw) {
            m_context.supportsMultiDraw = true;
            multiDrawFeatures.multiDraw = VK_TRUE;
            deviceExtensions.push_back(VK_EXT_MULTI_DRAW_EXTENSION_NAME);

            VkPhysicalDeviceMultiDrawPropertiesEXT multiDrawProps{};
            multiDrawProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT;

            VkPhysicalDeviceProperties2 props2{};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props2.pNext = &multiDrawProps;

            vkGetPhysicalDeviceProperties2(m_context.physicalDevice, &props2);
            m_context.maxMultiDrawCount = multiDrawProps.maxMultiDrawCount;
        } else {
            m_context.supportsMultiDraw = false;
            multiDrawFeatures.multiDraw = VK_FALSE;
        }

        if (extendedDynamicState2Features.extendedDynamicState2) {
            extendedDynamicState2Features.extendedDynamicState2 = VK_TRUE;
        } else {
            extendedDynamicState2Features.extendedDynamicState2 = VK_FALSE;
        }
        extendedDynamicState2Features.extendedDynamicState2LogicOp =
            extendedDynamicState2Features.extendedDynamicState2LogicOp ? VK_TRUE : VK_FALSE;
        extendedDynamicState2Features.extendedDynamicState2PatchControlPoints =
            extendedDynamicState2Features.extendedDynamicState2PatchControlPoints ? VK_TRUE : VK_FALSE;

        VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        deviceCreateInfo.pNext = &deviceFeatures2;
        deviceCreateInfo.pEnabledFeatures = nullptr;

        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_context.physicalDevice, &deviceCreateInfo, nullptr, &m_context.device) != VK_SUCCESS) {
            throw_error("Failed to create logical device");
        }

        log(" ======= Supported Features =======");
        log("GPU:");
        log("supportsMultiDraw: %s", m_context.supportsMultiDraw ? "true" : "false");
        log("supportsIndirectDraw: %s", m_context.supportsIndirectDraw ? "true" : "false");
        log("supportsBindlessTextures: %s", m_context.supportsBindlessTextures ? "true" : "false");
        log("supportsShaderDrawParameters: %s", m_context.supportsShaderDrawParameters ? "true" : "false");
        log("CPU:");
        log("supports AVX2: %s", HardwareInfo::HasAVX2() ? "true" : "false");
        log(" ==================================");

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
        m_p_resources = std::make_unique<VulkanResources>(m_context, m_vfs);

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

        std::string opaqueFrag = m_context.supportsBindlessTextures
                                 ? "Engine/shaders/OpaqueFragBindless.spv"
                                 : "Engine/shaders/OpaqueFrag.spv";

        m_p_pipeline->createGraphicsPipeline(
            "Engine/shaders/OpaqueVert.spv",
            opaqueFrag,
            bindingDesc,
            attributes
        );

        log("Initializing Masked Pipeline...");
        m_p_maskPipeline = std::make_unique<VulkanPipeline>(m_context);

        std::string maskedFrag = m_context.supportsBindlessTextures
                                 ? "Engine/shaders/MaskedFragBindless.spv"
                                 : "Engine/shaders/MaskedFrag.spv";

        m_p_maskPipeline->createMaskedPipeline(
            "Engine/shaders/MaskedVert.spv",
            maskedFrag,
            bindingDesc,
            attributes
        );

        log("Initializing Transparent Pipeline...");
        m_p_transPipeline = std::make_unique<VulkanPipeline>(m_context);

        std::string transFrag = m_context.supportsBindlessTextures
                                ? "Engine/shaders/TransparentFragBindless.spv"
                                : "Engine/shaders/TransparentFrag.spv";

        m_p_transPipeline->createTransparentPipeline(
            "Engine/shaders/TransparentVert.spv",
            transFrag,
            bindingDesc,
            attributes
        );

        log("Initializing UI Pipeline...");
        m_p_uiPipeline = std::make_unique<VulkanPipeline>(m_context);

        VkVertexInputBindingDescription uiBinding{};
        uiBinding.binding   = 0;
        uiBinding.stride    = sizeof(UIVertex);
        uiBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> uiAttrs(4);
        uiAttrs[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(UIVertex, position)};
        uiAttrs[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(UIVertex, uv)};
        uiAttrs[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(UIVertex, color)};
        uiAttrs[3] = {3, 0, VK_FORMAT_R32_SFLOAT,         offsetof(UIVertex, texIndex)};

        m_p_uiPipeline->createUIPipeline(
            "Engine/shaders/UiVert.spv",
            "Engine/shaders/UiFrag.spv",
            uiBinding,
            uiAttrs
        );

        log("Initializing Fullscreen Pipeline...");
        m_p_fullscreenPipeline = std::make_unique<VulkanPipeline>(m_context);

        VkVertexInputBindingDescription emptyBinding{};
        std::vector<VkVertexInputAttributeDescription> emptyAttrs;

        m_p_fullscreenPipeline->createFullscreenPipeline(
            "Engine/shaders/ScreenVert.spv",
            "Engine/shaders/ScreenFrag.spv"
        );

        #if DEBUG
            log("Initializing Physics Debug...");
            m_p_physicsDebug = std::make_unique<VulkanPhysicsDebug>();

            m_p_debugPipeline = std::make_unique<VulkanPipeline>(m_context);
            m_p_debugPipeline->createDebugPipeline("Engine/shaders/DebugVert.spv", "Engine/shaders/DebugFrag.spv");
        #endif

        log("Initializing Renderer...");
        m_p_renderer = std::make_unique<Renderer>(
            m_context,
            m_p_resources,
            m_p_pipeline,
            m_p_transPipeline,
            m_p_maskPipeline,
            m_p_uiPipeline,
            m_p_fullscreenPipeline,
            m_p_swapchainManager,
            m_p_meshManager
        );

        #if DEBUG
            m_p_renderer->setDebugPipeline(&m_p_debugPipeline);
        #endif

        log("Vulkan interface initialized successfully");

        } catch (const std::exception& e) {
            handle_critical_exception(e);
        }
    }

    Interface::~Interface() {
        vkDeviceWaitIdle(m_context.device);

        m_p_renderer.reset();
        m_p_meshManager.reset();
        m_p_resources.reset();
        m_p_pipeline.reset();
        m_p_transPipeline.reset();
        m_p_maskPipeline.reset();
        m_p_uiPipeline.reset();
        m_p_fullscreenPipeline.reset();
        #if DEBUG
        m_p_debugPipeline.reset();
        m_p_physicsDebug.reset();
        #endif
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

    void Interface::WaitForGPUToFinish() {
        vkDeviceWaitIdle(m_context.device);
    }

    void Interface::setVSync(bool enabled) {
        m_p_swapchainManager->setVSync(enabled);
        vkDeviceWaitIdle(m_context.device);
        m_context.requestSwapchainRecreation = true;
        //m_p_swapchainManager->recreateSwapchain();
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
}
