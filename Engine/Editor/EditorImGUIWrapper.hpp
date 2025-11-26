#pragma once
#include <imgui.h>
#include "../Core/src/components/backends/vulkan/VulkanImGUIWrapper.hpp"
#include "../Core/src/components/backends/vulkan/context.hpp"

class EditorImGUIWrapper : public vex::VulkanImGUIWrapper {
public:

    EditorImGUIWrapper(SDL_Window* window, vex::VulkanContext& vulkanContext)
        : vex::VulkanImGUIWrapper(window, vulkanContext) {
    }

    void processEvent(const SDL_Event* event) override;
};
