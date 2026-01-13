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
#include "PhysicsDebug.hpp"
#include "Resources.hpp"
#include "Pipeline.hpp"
#include "MeshManager.hpp"
#include "Renderer.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <components/GameInfo.hpp>
#include <components/UI/UIVertex.hpp>
#include "components/VirtualFileSystem.hpp"
#include "components/enviroment.hpp"

namespace vex {
    /// @brief Interface Class for vulkan backend.
    /// @details This class manages the entire Vulkan backend lifecycle. It handles:
    /// - Instance and Device creation (selecting GPU, enabling extensions like Dynamic Rendering/MultiDraw).
    /// - Window surface binding and Swapchain initialization via `VulkanSwapchainManager`.
    /// - Resource management (Textures, UBOs) via `VulkanResources`.
    /// - Pipeline creation (Graphics, UI, Fullscreen) via `VulkanPipeline`.
    class Interface {
    public:
        /// @brief Constructor for Interface class.
        /// @details Initializes the Vulkan loader, creates an Instance with required extensions (including Portability for macOS), picks a physical device, creates a logical device with VMA allocator, and initializes all subsystems (Resources, MeshManager, Pipelines, Renderer).
        /// @param SDL_Window* window - Pointer to the SDL_Window to bind to.
        /// @param glm::uvec2 initialResolution - Initial rendering resolution.
        /// @param GameInfo gInfo - Game metadata used for application info in Vulkan.
        /// @param VirtualFileSystem* vfs - Pointer to the virtual file system for asset loading.
        Interface(SDL_Window* window, glm::uvec2 initialResolution, GameInfo gInfo, VirtualFileSystem* vfs);

        /// @brief Simple destructor cleaning up resources.
        /// @details Waits for the device to idle, then destroys subsystems in reverse dependency order (Renderer -> Pipelines -> Resources -> Swapchain -> Device -> Instance).
        ~Interface();

        /// @brief Binds the backend to a window.
        /// @details Creates a Vulkan surface using `SDL_Vulkan_CreateSurface` and initializes the swapchain.
        /// @param SDL_Window* window - Pointer to the SDL_Window.
        void bindWindow(SDL_Window* window);

        /// @brief Unbinds the current window.
        /// @details Waits for the GPU to idle, cleans up the swapchain, and destroys the window surface.
        void unbindWindow();

        /// @brief Creates the default texture.
        /// @details Delegates to `VulkanResources::createDefaultTexture` to generate a 1x1 white fallback texture.
        void createDefaultTexture();

        /// @brief Getter for VulkanContext.
        /// @return VulkanContext* - Pointer to the core Vulkan context structure (Device, Queue, etc.).
        VulkanContext* getContext() { return &m_context; }

        /// @brief Getter for VulkanResources.
        /// @return VulkanResources* - Pointer to the resource manager.
        VulkanResources* getResources() { return m_p_resources.get(); }

        /// @brief Getter for MeshManager.
        /// @return MeshManager& - Reference to the mesh manager.
        MeshManager& getMeshManager() { return *m_p_meshManager; }

        /// @brief Getter for Renderer.
        /// @return Renderer& - Reference to the renderer.
        Renderer& getRenderer() { return *m_p_renderer; }

        /// @brief Allows to update Enviroment settings at runtime.
        /// @details Updates the internal environment struct in the context, which the Renderer later pushes to the Scene UBO (e.g., Lighting, PS1 effects).
        /// @param enviroment env - The new environment settings.
        void setEnvironment(enviroment env) {m_context.m_enviroment = env;}

        /// @brief Getter for Enviroment settings.
        /// @return enviroment - A copy of the current environment settings.
        enviroment getEnvironment() { return m_context.m_enviroment;}

        /// @brief Helper function to wait for GPU to finish.
        /// @details Calls `vkDeviceWaitIdle`. Should be used before resizing or shutting down to prevent resource hazards.
        void WaitForGPUToFinish();

        /// @brief Sets VSync (Vertical Synchronization).
        /// @details Updates the swapchain manager settings and requests a swapchain recreation on the next frame.
        /// @param bool enabled - True to enable VSync (FIFO), false to disable (IMMEDIATE/MAILBOX).
        void setVSync(bool enabled);

        #if DEBUG
        /// @brief Getter for Physics Debug Renderer.
        /// @return VulkanPhysicsDebug* - Pointer to the debug renderer instance.
        VulkanPhysicsDebug* getPhysicsDebug() { return m_p_physicsDebug.get(); }
        #endif
    private:
        VulkanContext m_context;
        SDL_Window* m_p_window;
        VirtualFileSystem* m_vfs;
        std::unique_ptr<VulkanSwapchainManager> m_p_swapchainManager;
        std::unique_ptr<VulkanResources> m_p_resources;
        std::unique_ptr<VulkanPipeline> m_p_pipeline;
        std::unique_ptr<VulkanPipeline> m_p_transPipeline;
        std::unique_ptr<VulkanPipeline> m_p_maskPipeline;
        std::unique_ptr<VulkanPipeline> m_p_uiPipeline;
        std::unique_ptr<VulkanPipeline> m_p_fullscreenPipeline;
        std::unique_ptr<MeshManager> m_p_meshManager;
        std::unique_ptr<Renderer> m_p_renderer;

        #if DEBUG
            std::unique_ptr<VulkanPipeline> m_p_debugPipeline;
            std::unique_ptr<VulkanPhysicsDebug> m_p_physicsDebug;
        #endif
    };
}
