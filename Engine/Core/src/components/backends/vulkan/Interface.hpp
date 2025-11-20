/**
 *  @file   Interface.hpp
 *  @brief  This file defines interface Class for vulkan backend.
 *  @author Eryk Roszkowski
 ***********************************************/

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
#include <components/UI/UIVertex.hpp>
#include "../../VirtualFileSystem.hpp"
#include "components/enviroment.hpp"

namespace vex {
    /// @brief Interface Class for vulkan backend.
    /// @details This class manages whole vulkan backend, from initalization, to holding resources, binding window, to calling renderer. Its managing class for backend abstraction. Its architecture allows me to replace it with for example OpenGL Interface with same methods and interface should manage backend by itself.
    class Interface {
    public:
        /// @brief Constructor for Interface class.
        /// @param SDL_Window* window - Pointer to SDL_Window.
        /// @param glm::uvec2 initialResolution - Initial resolution of the window.
        /// @param GameInfo gInfo - GameInfo object.
        /// @param VirtualFileSystem* vfs - Pointer to VirtualFileSystem.
        /// @details It initializes vulkan backend, binds window, creates resources, and initializes renderer.
        Interface(SDL_Window* window, glm::uvec2 initialResolution, GameInfo gInfo, VirtualFileSystem* vfs);

        /// @brief Simple destructor cleaning up resources.
        ~Interface();

        /// @brief Binds window.
        /// @param SDL_Window* window - Pointer to SDL_Window.
        /// @details It is used internally to bind backend to the window. Its called when initializing backend. And everytime the window is resized.
        void bindWindow(SDL_Window* window);

        /// @brief Simply unbinds window.
        void unbindWindow();

        /// @brief Updates render resolution and manages window size change.
        void setRenderResolution(glm::uvec2 resolution);

        /// @brief Creates default texture for untextured meshes. (to be exact, it calls VulkanResources method for it)
        void createDefaultTexture();

        /// @brief Getter for VulkanContext.
        /// @return VulkanContext*
        VulkanContext* getContext() { return &m_context; }

        /// @brief Getter for VulkanResources.
        /// @return VulkanResources*
        VulkanResources* getResources() { return m_p_resources.get(); }

        /// @brief Getter for MeshManager.
        /// @return MeshManager&
        MeshManager& getMeshManager() { return *m_p_meshManager; }

        /// @brief Getter for Renderer.
        /// @return Renderer&
        Renderer& getRenderer() { return *m_p_renderer; }

        /// @brief Allows to update Enviroment settings at runtime. Since they are in push constant we need interface to extract and pass the data.
        /// @param enviroment env
        void setEnvironment(enviroment env) {m_context.m_enviroment = env;}

        /// @brief Getter for Enviroment settings. Returns a copy of the current environment settings.
        /// @return enviroment
        enviroment getEnvironment() { return m_context.m_enviroment;}

    private:
        VulkanContext m_context;
        SDL_Window* m_p_window;
        VirtualFileSystem* m_vfs;
        std::unique_ptr<VulkanSwapchainManager> m_p_swapchainManager;
        std::unique_ptr<VulkanResources> m_p_resources;
        std::unique_ptr<VulkanPipeline> m_p_pipeline;
        std::unique_ptr<VulkanPipeline> m_p_transPipeline;
        std::unique_ptr<VulkanPipeline> m_p_uiPipeline;
        std::unique_ptr<MeshManager> m_p_meshManager;
        std::unique_ptr<Renderer> m_p_renderer;
    };
}
