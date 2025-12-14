#include "EditorImGUIWrapper.hpp"
#include "Editor.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <ImGuizmo.h>

void EditorImGUIWrapper::beginFrame(){
    vex::VulkanImGUIWrapper::beginFrame();
    ImGuizmo::BeginFrame();
}

void EditorImGUIWrapper::processEvent(const SDL_Event* event) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL3_ProcessEvent(event);
}
