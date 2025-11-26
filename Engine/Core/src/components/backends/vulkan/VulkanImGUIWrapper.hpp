/**
 *  @file   VulkanImGUIWrapper.hpp
 *  @brief  This file defines class with Imgui for vulkan backend definition.
 *  @author Eryk Roszkowski
 ***********************************************/

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
    /// @brief Class with Imgui for vulkan backend definition, inheriting from ImGUIWrapper.
    class VulkanImGUIWrapper : public ImGUIWrapper {
    public:
        /// @brief Constructor for VulkanImGUIWrapper.
        /// @param SDL_Window* window - The SDL window to use for ImGUI.
        /// @param VulkanContext& vulkanContext - The Vulkan context containing vulkan resources..
        VulkanImGUIWrapper(SDL_Window* window, VulkanContext& vulkanContext);
        virtual ~VulkanImGUIWrapper();
        VkDescriptorSet addTexture(VkSampler sampler, VkImageView imageView, VkImageLayout layout);
#if DEBUG
        /// @brief Initializes ImGUI for Vulkan backend, called by the engine while initializing.
        void init() override;
        /// @brief Begins a new frame for ImGUI, needs to be called after render method.
        void beginFrame() override;
        /// @brief Ends the current frame for ImGUI, needs to be called after render method.
        void endFrame() override;
        /// @brief Processes an mouse events.
        void processEvent(const SDL_Event* event) override;
        /// @brief Gets the ImGuiIO instance.
        /// @return ImGuiIO& - The ImGuiIO instance.
        ImGuiIO& getIO() override;
        void draw(VkCommandBuffer commandBuffer);
#else
    void init() override { return; }
    void beginFrame() override { return; }
    void endFrame() override { return; }
    void processEvent(const SDL_Event* event) override { return; }
    void draw(VkCommandBuffer commandBuffer) { return; }
#endif


    private:
#if DEBUG
        /// @brief Creates the descriptor pool for ImGUI.
        void createDescriptorPool();
        /// @brief Sets up the style for ImGUI.
        void setupStyle();

        VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
        bool m_initialized = false;
#endif
        SDL_Window *m_p_window;
        VulkanContext& m_r_context;
    };
}
