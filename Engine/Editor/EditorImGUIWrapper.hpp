/**
 * @file   EditorImGUIWrapper.hpp
 * @brief  A specialized ImGUI wrapper for the editor, inheriting from VulkanImGUIWrapper.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <imgui.h>
#include "../Core/src/components/backends/vulkan/VulkanImGUIWrapper.hpp"
#include "../Core/src/components/backends/vulkan/context.hpp"

/// @brief A specialized ImGUI wrapper for the editor, which customizes initialization for editor features.
class EditorImGUIWrapper : public vex::VulkanImGUIWrapper {
public:

    /**
     * @brief Constructs an EditorImGUIWrapper.
     * @param SDL_Window* window - The main SDL window.
     * @param vex::VulkanContext& vulkanContext - The Vulkan context reference.
     */
    EditorImGUIWrapper(SDL_Window* window, vex::VulkanContext& vulkanContext)
        : vex::VulkanImGUIWrapper(window, vulkanContext) {}

    /// @brief Initializes ImGUI for the editor, setting a custom layout file.
    void init() override {
            vex::VulkanImGUIWrapper::init();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = "editorLayout.ini";
        }

    /// @brief Overrides the base class function to begin a new ImGUI frame.
    void beginFrame() override;

    /// @brief Processes SDL events specifically for ImGUI interaction within the editor.
    /// @param const SDL_Event* event - The SDL event to process.
    void processEvent(const SDL_Event* event) override;
};
