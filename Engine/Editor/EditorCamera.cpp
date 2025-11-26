#include "EditorCamera.hpp"
#include <components/GameComponents/BasicComponents.hpp>

void EditorCameraObject::processEvent(const SDL_Event& event, float deltaTime) {
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        bool isPressed = (event.type == SDL_EVENT_KEY_DOWN);
        const SDL_KeyboardEvent& key_event = event.key;
        keyStates[key_event.scancode] = isPressed;
    }

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        if (event.wheel.y > 0) {
            m_movementSpeed *= 1.2f;
        } else if (event.wheel.y < 0) {
            m_movementSpeed *= 0.8f;
        }
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event.button.button == SDL_BUTTON_RIGHT && m_viewportHovered) {
            m_isFlying = true;
            SDL_SetWindowRelativeMouseMode(m_window, true);
        }
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event.button.button == SDL_BUTTON_RIGHT) {
            m_isFlying = false;
            SDL_SetWindowRelativeMouseMode(m_window, false);
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION && m_isFlying) {
        auto& tc = GetComponent<vex::TransformComponent>();

        float deltaYaw = -event.motion.xrel * m_mouseSensitivity;
        float deltaPitch = -event.motion.yrel * m_mouseSensitivity;

        tc.addYaw(deltaYaw);
        tc.addPitch(deltaPitch);
    }
}

void EditorCameraObject::Update(float deltaTime) {
    auto& tc = GetComponent<vex::TransformComponent>();

    if (keyStates[SDL_SCANCODE_W]) {
        tc.setLocalPosition(tc.getLocalPosition()+(tc.getForwardVector()*float(deltaTime*m_movementSpeed)));
    }

    if (keyStates[SDL_SCANCODE_S]) {
        tc.setLocalPosition(tc.getLocalPosition()+(tc.getForwardVector()*float(deltaTime*-m_movementSpeed)));
    }

    if (keyStates[SDL_SCANCODE_D]) {
        tc.setLocalPosition(tc.getLocalPosition()+(tc.getRightVector()*float(deltaTime*m_movementSpeed)));
    }

    if (keyStates[SDL_SCANCODE_A]) {
        tc.setLocalPosition(tc.getLocalPosition()+(tc.getRightVector()*float(deltaTime*-m_movementSpeed)));
    }

    if (keyStates[SDL_SCANCODE_E]) {
        tc.setLocalPosition(tc.getLocalPosition() + (tc.getUpVector() * (deltaTime * m_movementSpeed)));
    }

    if (keyStates[SDL_SCANCODE_Q]) {
        tc.setLocalPosition(tc.getLocalPosition() + (tc.getUpVector() * (deltaTime * -m_movementSpeed)));
    }
}
