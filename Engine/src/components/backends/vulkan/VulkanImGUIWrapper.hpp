#pragma once

#include "components/ImGUIWrapper.hpp"
#include "components/errorUtils.hpp"
#include "context.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#if DEBUG
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#endif

namespace vex {
    class VulkanImGUIWrapper : public ImGUIWrapper {
    public:
        VulkanImGUIWrapper(SDL_Window* window, VulkanContext& vulkanContext);
        virtual ~VulkanImGUIWrapper();
#if DEBUG
        void init() override;
        void beginFrame() override;
        void endFrame() override;
        void processEvent(const SDL_Event* event) override;
        ImGuiIO& getIO() override;
#else
    void init() override { return; }
    void beginFrame() override { return; }
    void endFrame() override { return; }
    void processEvent(const SDL_Event* event) override { return; }
#endif

    private:
#if DEBUG
        void createDescriptorPool();
        void setupStyle();

        VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
        bool m_initialized = false;
#endif
        SDL_Window *m_p_window;
        VulkanContext& m_r_context;
    };
}
