#include "VulkanImGUIWrapper.hpp"

#if DEBUG
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#endif

namespace vex {

    VulkanImGUIWrapper::VulkanImGUIWrapper(SDL_Window* window, VulkanContext& vulkanContext)
        : m_p_window(window), m_r_context(vulkanContext) {}

    VulkanImGUIWrapper::~VulkanImGUIWrapper() {

#if DEBUG
    if (m_initialized) {
        vkDeviceWaitIdle(m_r_context.device);

        ImGui_ImplVulkan_DestroyFontsTexture();
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
#if DEBUG

    void VulkanImGUIWrapper::init() {
        log("Initialization of DearImGUI");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        if (!vkCreateSampler || !vkCreateDescriptorPool) {
            throw_error("Critical Vulkan function pointers are null!");
        }

        setupStyle();
        ImGui_ImplSDL3_InitForVulkan(m_p_window);

        createDescriptorPool();

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_r_context.instance;
        init_info.PhysicalDevice = m_r_context.physicalDevice;
        init_info.Device = m_r_context.device;
        init_info.QueueFamily = m_r_context.graphicsQueueFamily;
        init_info.Queue = m_r_context.graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.Subpass = 0;
        init_info.MinImageCount = m_r_context.swapchainImages.size();
        init_info.ImageCount = m_r_context.swapchainImages.size();
        init_info.CheckVkResultFn = [](VkResult err) {
            if (err != VK_SUCCESS) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ImGui Vulkan error: %d", err);
            }
        };
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.DescriptorPool = m_imguiPool;
        init_info.UseDynamicRendering = true;

        init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_r_context.swapchainImageFormat;
        init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = m_r_context.depthFormat;

        log("Initing ImGui Vulkan backend");
        if (!ImGui_ImplVulkan_Init(&init_info)) {
            throw_error("Failed to initialize ImGui Vulkan backend");
        }

        log("Creating ImGUI font textures");
        if (!ImGui_ImplVulkan_CreateFontsTexture()) {
            throw_error("Failed to create ImGui font textures");
        }

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
        //float scaleX = (float)m_r_context.currentRenderResolution.x / m_r_context.swapchainExtent.width;
        //float scaleY = (float)m_r_context.currentRenderResolution.y / m_r_context.swapchainExtent.height;
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    }

    void VulkanImGUIWrapper::endFrame() {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_r_context.commandBuffers[m_r_context.currentImageIndex]);
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
                // Forward the event with original coordinates
                // (ImGui will use our corrected MousePos)
                ImGui_ImplSDL3_ProcessEvent(event);
                break;
            }
            default: {
                // Forward all other events unchanged
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
        //const float base_scale = float(m_r_context.swapchainExtent.height / m_r_context.currentRenderResolution.y);

        ImGuiStyle& style = ImGui::GetStyle();

        //ImGuiIO& io = ImGui::GetIO();
        //io.Fonts->Clear();
        //io.FontGlobalScale = 1.0f;
        //io.Fonts->AddFontFromFileTTF("Engine/assets/fonts/editundo.ttf", 16.0f, &config);

        style.WindowPadding = ImVec2(2, 2);
        style.FramePadding = ImVec2(2, 2);
        style.ItemSpacing = ImVec2(2, 2);
        style.ItemInnerSpacing = ImVec2(2, 2);
        style.IndentSpacing = 4.0f;
        style.ScrollbarSize = 4.0f;
        style.GrabMinSize = 4.0f;

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    }
#endif
}
