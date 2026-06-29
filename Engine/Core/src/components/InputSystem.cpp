#include "components/InputSystem.hpp"
#include <SDL3/SDL.h>
#include <unordered_map>
#include "SDL3/SDL_mouse.h"
#include "components/GameComponents/InputComponent.hpp"


namespace vex {

    InputSystem::InputSystem(vex::Registry &registry, SDL_Window *window)
        : m_registry(registry), m_window(window), m_inputMode(InputMode::Game)
    {
        setInputMode(InputMode::Game);

        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
        {
            log(LogLevel::ERROR, "Could not initialize gamepad subsystem: %s", SDL_GetError());
        }
    }

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

    void InputSystem::processEvent(const SDL_Event &event, float deltaTime) {
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            bool isPressed = (event.type == SDL_EVENT_KEY_DOWN);
            SDL_Scancode scancode = event.key.scancode;

            auto& keyState = keyStates[scancode];
            bool wasPressed = keyState.isPressed;

            keyState.isPressed = isPressed;
            if (isPressed && !wasPressed) {
                keyState.wasProcessedAsPressed = false;
            }

            vex::View<InputComponent>(m_registry).each([&, this, scancode, isPressed, wasPressed](vex::Entity entity, InputComponent& inputComp) {
                for (const auto& binding : inputComp.bindings) {
                    if (binding.scancode == scancode) {
                        if (isPressed && binding.state == InputActionState::Pressed && !wasPressed && !keyStates[scancode].wasProcessedAsPressed) {
                            binding.action(deltaTime);
                            keyStates[scancode].wasProcessedAsPressed = true;
                        } else if (!isPressed && binding.state == InputActionState::Released) {
                            binding.action(deltaTime);
                        } else if (!isPressed && binding.state == InputActionState::Held) {
                            binding.action(0.0f);
                        }
                    }
                }
            });
        } else if (event.type == SDL_EVENT_MOUSE_MOTION && m_inputMode != InputMode::UI) {
            vex::View<InputComponent>(m_registry).each([&, this](vex::Entity entity, InputComponent& inputComp) {
                for (const auto& binding : inputComp.mouseAxisBindings) {
                    float delta = (binding.axis == MouseAxis::X) ? event.motion.xrel : event.motion.yrel;
                    binding.action(delta);
                }
            });
        } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
            bool isPressed = (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            SDL_GamepadButton button = (SDL_GamepadButton)event.gbutton.button;

            gamepadButtonStates[button].isPressed = isPressed;

            vex::View<InputComponent>(m_registry).each([&, this, button, isPressed, deltaTime](vex::Entity entity, InputComponent& inputComp) {
                for (const auto& binding : inputComp.gamepadButtonBindings) {
                    if (binding.button == button) {
                        if (isPressed && binding.state == InputActionState::Pressed) {
                            binding.action(deltaTime);
                        } else if (!isPressed && binding.state == InputActionState::Released) {
                            binding.action(deltaTime);
                        }
                    }
                }
            });
        } else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            float normalizedValue = event.gaxis.value / 32767.0f;
            gamepadAxisStates[(SDL_GamepadAxis)event.gaxis.axis] = normalizedValue;
        }
    }

    void InputSystem::update(float deltaTime) {
        vex::View<InputComponent>(m_registry).each([&, this, deltaTime](vex::Entity entity, InputComponent& inputComp) {
            for (const auto& binding : inputComp.gamepadAxisBindings) {
                float currentValue = gamepadAxisStates[binding.axis];
                binding.action(currentValue);
            }
            for (const auto& binding : inputComp.gamepadButtonBindings) {
                if (binding.state == InputActionState::Held && gamepadButtonStates[binding.button].isPressed) {
                    binding.action(deltaTime);
                }
            }
            for (const auto& binding : inputComp.bindings) {
                if (binding.state == InputActionState::Held && keyStates[binding.scancode].isPressed) {
                    binding.action(deltaTime);
                }
            }
        });
    }
}