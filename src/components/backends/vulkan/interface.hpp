#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <memory>

//Renderer
#include "context.hpp"
#include "swapchain_manager.hpp"

namespace vex {
    class Interface {
    private:
        VulkanContext context;
        bool shouldClose;

        SDL_Window *m_window;
        std::unique_ptr<VulkanSwapchainManager> m_swapchain_manager;
    public:
        Interface(SDL_Window* window);
        ~Interface();

        void bindWindow(SDL_Window* window);
        void unbindWindow();
        void renderFrame();
    };
}
