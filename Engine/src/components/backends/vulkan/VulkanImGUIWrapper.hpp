#pragma once

#include "components/ImGUIWrapper.hpp"
#include "components/errorUtils.hpp"
#include "context.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <imgui.h>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

namespace vex {
    class VulkanImGUIWrapper : public ImGUIWrapper {
    public:
        VulkanImGUIWrapper(SDL_Window* window, VulkanContext& vulkanContext);
        virtual ~VulkanImGUIWrapper();

        void init() override;
        void beginFrame() override;
        void endFrame() override;
        void processEvent(const SDL_Event* event) override;
        ImGuiIO& getIO() override;

    private:
        void createDescriptorPool();
        void setupStyle();

        SDL_Window* m_window;
        VulkanContext& m_vulkanContext;
        VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
        bool m_initialized = false;
    };
}
