#pragma once

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "context.hpp"
#include "SwapchainManager.hpp"
#include "Resources.hpp"
#include "Pipeline.hpp"
#include "MeshManager.hpp"
#include "Renderer.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <components/GameInfo.hpp>
#include "../../VirtualFileSystem.hpp"
#include "components/enviroment.hpp"

namespace vex {
    class Interface {
    public:
        Interface(SDL_Window* window, glm::uvec2 initialResolution, GameInfo gInfo, VirtualFileSystem* vfs);
        ~Interface();

        void bindWindow(SDL_Window* window);
        void unbindWindow();
        void setRenderResolution(glm::uvec2 resolution);
        void createDefaultTexture();

        VulkanContext* getContext() { return &m_context; }
        MeshManager& getMeshManager() { return *m_p_meshManager; }
        Renderer& getRenderer() { return *m_p_renderer; }

        void setEnvironment(enviroment env) {m_context.m_enviroment = env;}
        enviroment getEnvironment() { return m_context.m_enviroment;}

    private:
        VulkanContext m_context;
        SDL_Window* m_p_window;
        VirtualFileSystem* m_vfs;
        std::unique_ptr<VulkanSwapchainManager> m_p_swapchainManager;
        std::unique_ptr<VulkanResources> m_p_resources;
        std::unique_ptr<VulkanPipeline> m_p_pipeline;
        std::unique_ptr<MeshManager> m_p_meshManager;
        std::unique_ptr<Renderer> m_p_renderer;
    };
}
