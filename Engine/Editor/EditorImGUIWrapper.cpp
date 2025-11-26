#include "EditorImGUIWrapper.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

void EditorImGUIWrapper::processEvent(const SDL_Event* event) {
    ImGuiIO& io = ImGui::GetIO();

    float scaleX = 1;
    float scaleY = 1;

    switch (event->type) {
        case SDL_EVENT_MOUSE_MOTION: {
            io.MousePos = ImVec2(
                event->motion.x / scaleX,
                event->motion.y / scaleY
            );
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            io.MousePos = ImVec2(
                event->button.x / scaleX,
                event->button.y / scaleY
            );
            ImGui_ImplSDL3_ProcessEvent(event);
            break;
        }
        default: {
            ImGui_ImplSDL3_ProcessEvent(event);
            break;
        }
    }
}
