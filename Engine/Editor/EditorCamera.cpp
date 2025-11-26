#include "EditorCamera.hpp"
#include <components/GameComponents/BasicComponents.hpp>

void EditorCameraObject::processEvent(const SDL_Event& event, float deltaTime) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        const SDL_KeyboardEvent& key_event = event.key;
        auto tc = GetComponent<vex::TransformComponent>();

        if (key_event.scancode == SDL_SCANCODE_W) {
            tc.setLocalPosition(tc.getLocalPosition()+(tc.getForwardVector()*float(deltaTime*250000)));
        }

        if (key_event.scancode == SDL_SCANCODE_S) {
            tc.setLocalPosition(tc.getLocalPosition()+(tc.getForwardVector()*float(deltaTime*-250000)));
        }

        if (key_event.scancode == SDL_SCANCODE_D) {
            tc.setLocalPosition(tc.getLocalPosition()+(tc.getRightVector()*float(deltaTime*250000)));
        }

        if (key_event.scancode == SDL_SCANCODE_A) {
            tc.setLocalPosition(tc.getLocalPosition()+(tc.getRightVector()*float(deltaTime*-250000)));
        }
    }
}
