#include "VulkanImGUIWrapper.hpp"

#if DEBUG
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <filesystem>
#include <ImGuizmo.h>
#endif

namespace vex {

    VulkanImGUIWrapper::VulkanImGUIWrapper(SDL_Window* window, VulkanContext& vulkanContext)
        : m_p_window(window), m_r_context(vulkanContext) {}

    VulkanImGUIWrapper::~VulkanImGUIWrapper() {
#if DEBUG
        if (m_initialized) {
            vkDeviceWaitIdle(m_r_context.device);

            ImGui_ImplVulkan_Shutdown();

            if (m_imguiPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(m_r_context.device, m_imguiPool, nullptr);
                m_imguiPool = VK_NULL_HANDLE;
            }

            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
        }
#endif
    }

    VkDescriptorSet VulkanImGUIWrapper::addTexture(VkSampler sampler, VkImageView imageView, VkImageLayout layout) {
    #if DEBUG
        return ImGui_ImplVulkan_AddTexture(sampler, imageView, layout);
    #else
        return VK_NULL_HANDLE;
    #endif
    }

#if DEBUG
    void VulkanImGUIWrapper::init() {
        log("Initialization of DearImGUI");

        volatile auto forceLink = ImGuizmo::BeginFrame;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        if(std::filesystem::exists("../Assets/Sono-SemiBold.ttf")){
            ImFont* myFont = io.Fonts->AddFontFromFileTTF("../Assets/Sono-SemiBold.ttf", 14.0f);

            if (myFont == nullptr) {
                log("Failed to load editor font!");
                io.Fonts->AddFontDefault();
            }
        }else{
            log("Failed to load editor font!");
            io.Fonts->AddFontDefault();
        }

        if (!vkCreateSampler || !vkCreateDescriptorPool) {
            throw_error("Critical Vulkan function pointers are null!");
        }

        ImGui_ImplSDL3_InitForVulkan(m_p_window);

        createDescriptorPool();

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_r_context.instance;
        init_info.PhysicalDevice = m_r_context.physicalDevice;
        init_info.Device = m_r_context.device;
        init_info.QueueFamily = m_r_context.graphicsQueueFamily;
        init_info.Queue = m_r_context.graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.MinImageCount = m_r_context.swapchainImages.size();
        init_info.ImageCount = m_r_context.swapchainImages.size();
        init_info.CheckVkResultFn = [](VkResult err) {
            if (err != VK_SUCCESS) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ImGui Vulkan error: %d", err);
            }
        };
        init_info.Allocator = nullptr;
        init_info.DescriptorPool = m_imguiPool;
        init_info.UseDynamicRendering = true;

        // Setup for dynamic rendering
        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &m_r_context.swapchainImageFormat;
        pipelineRenderingCreateInfo.depthAttachmentFormat = m_r_context.depthFormat;

        ImGui_ImplVulkan_PipelineInfo pipelineInfoMain = {};
        pipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;

        init_info.PipelineInfoMain = pipelineInfoMain;

        log("Initing ImGui Vulkan backend");
        if (!ImGui_ImplVulkan_Init(&init_info)) {
            throw_error("Failed to initialize ImGui Vulkan backend");
        }

        setupStyle();

        m_initialized = true;
    }

    void VulkanImGUIWrapper::beginFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(
            (float)m_r_context.currentRenderResolution.x,
            (float)m_r_context.currentRenderResolution.y
        );
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    }

    void VulkanImGUIWrapper::endFrame() {
        ImGui::Render();

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void VulkanImGUIWrapper::draw(VkCommandBuffer commandBuffer) {
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData) {
            ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        }
    }

    void VulkanImGUIWrapper::processEvent(const SDL_Event* event) {
        ImGuiIO& io = ImGui::GetIO();

        // Calculate scale factors (window-to-render)
        float scaleX = (float)m_r_context.swapchainExtent.width /
                      (float)m_r_context.currentRenderResolution.x;
        float scaleY = (float)m_r_context.swapchainExtent.height /
                      (float)m_r_context.currentRenderResolution.y;

        switch (event->type) {
            case SDL_EVENT_MOUSE_MOTION: {
                io.MousePos = ImVec2(
                    event->motion.x / scaleX,
                    event->motion.y / scaleY
                );
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                io.MousePos = ImVec2(
                    event->button.x / scaleX,
                    event->button.y / scaleY
                );
                ImGui_ImplSDL3_ProcessEvent(event);
                break;
            }
            default: {
                ImGui_ImplSDL3_ProcessEvent(event);
                break;
            }
        }
    }

    ImGuiIO& VulkanImGUIWrapper::getIO() {
        return ImGui::GetIO();
    }

    void VulkanImGUIWrapper::createDescriptorPool() {
        log("Creating ImGUI DescriptorPools");

        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        if (vkCreateDescriptorPool(m_r_context.device, &pool_info, nullptr, &m_imguiPool) != VK_SUCCESS) {
            throw_error("Failed to create descriptor pool for ImGui");
        }
    }

    void VulkanImGUIWrapper::setupStyle() {
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(5, 5);
        style.ItemSpacing = ImVec2(6, 6);
        style.ItemInnerSpacing = ImVec2(4, 4);
        style.IndentSpacing = 4.0f;
        style.ScrollbarSize = 4.0f;
        style.GrabMinSize = 4.0f;

        style.WindowRounding = 6.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 6.0f;

        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.03f, 0.03f, 0.03f, 0.75f);
        colors[ImGuiCol_ChildBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.75f);
        colors[ImGuiCol_Border]                 = ImVec4(0.06f, 0.06f, 0.06f, 0.10f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.21f, 0.22f, 0.10f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 0.20f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.10f, 0.10f, 0.10f, 0.40f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.31f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.20f, 0.21f, 0.22f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    }
#endif
}
