#include "components/InputSystem.hpp"
#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <unordered_map>
#include "SDL3/SDL_mouse.h"
#include "components/GameComponents/InputComponent.hpp"


namespace vex {

    void InputSystem::setInputMode(InputMode mode) {
        m_inputMode = mode;
        if (mode == InputMode::Game) {
            SDL_SetWindowRelativeMouseMode(m_window, true);
            SDL_HideCursor();
        } else {
            SDL_SetWindowRelativeMouseMode(m_window, false);
            SDL_ShowCursor();
        }
    }


    void InputSystem::processEvent(const SDL_Event& event, float deltaTime) {
        if (m_inputMode == InputMode::UI && event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) {
            return;
        }

        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            SDL_Scancode scancode = event.key.scancode;
            bool isPressed = (event.type == SDL_EVENT_KEY_DOWN);

            auto& keyState = keyStates[scancode];
            bool wasPressed = keyState.isPressed;

            keyState.isPressed = isPressed;
            if (isPressed && !wasPressed) {
                keyState.wasProcessedAsPressed = false;
            }

            auto view = m_registry.view<InputComponent>();
            for (auto entity : view) {
                auto& inputComp = view.get<InputComponent>(entity);
                for (const auto& binding : inputComp.bindings) {
                    if (binding.scancode == scancode) {
                        if (isPressed && binding.state == InputActionState::Pressed && !wasPressed && !keyState.wasProcessedAsPressed) {
                            binding.action(deltaTime);
                            keyState.wasProcessedAsPressed = true;
                        } else if (!isPressed && binding.state == InputActionState::Released) {
                            binding.action(deltaTime);
                        }
                    }
                }
            }
        } else if (event.type == SDL_EVENT_MOUSE_MOTION && m_inputMode != InputMode::UI) {
            auto view = m_registry.view<InputComponent>();
            for (auto entity : view) {
                auto& inputComp = view.get<InputComponent>(entity);
                for (const auto& binding : inputComp.mouseAxisBindings) {
                    float delta = (binding.axis == MouseAxis::X) ? event.motion.xrel : event.motion.yrel;
                    binding.action(delta);
                }
            }
        } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
            bool isPressed = (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            SDL_GamepadButton button = (SDL_GamepadButton)event.gbutton.button;

            gamepadButtonStates[button].isPressed = isPressed;

            auto view = m_registry.view<InputComponent>();
            for (auto entity : view) {
                auto& inputComp = view.get<InputComponent>(entity);
                for (const auto& binding : inputComp.gamepadButtonBindings) {
                    if (binding.button == button) {
                        if (isPressed && binding.state == InputActionState::Pressed) {
                            binding.action(deltaTime);
                        } else if (!isPressed && binding.state == InputActionState::Released) {
                            binding.action(deltaTime);
                        }
                    }
                }
            }
        } else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            float normalizedValue = event.gaxis.value / 32767.0f;

            auto view = m_registry.view<InputComponent>();
            for (auto entity : view) {
                auto& inputComp = view.get<InputComponent>(entity);
                for (const auto& binding : inputComp.gamepadAxisBindings) {
                    if (binding.axis == (SDL_GamepadAxis)event.gaxis.axis) {
                        binding.action(normalizedValue);
                    }
                }
            }
        }
    }

    void InputSystem::update(float deltaTime) {
        auto view = m_registry.view<InputComponent>();
        for (auto entity : view) {
            auto& inputComp = view.get<InputComponent>(entity);
            for (const auto& binding : inputComp.bindings) {
                if (binding.state == InputActionState::Held && keyStates[binding.scancode].isPressed) {
                    binding.action(deltaTime);
                }
            }
            for (const auto& binding : inputComp.gamepadButtonBindings) {
                if (binding.state == InputActionState::Held && gamepadButtonStates[binding.button].isPressed) {
                    binding.action(deltaTime);
                }
            }
        }
    }
}
