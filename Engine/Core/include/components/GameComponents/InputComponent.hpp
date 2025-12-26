/**
 *  @file   InputComponent.hpp
 *  @brief  Component you add to your GameObject to handle input, eg. add input bindings and mouse axis bindings.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vex {

/// @brief Enum representing the state of an input action.
enum class InputActionState {
    Pressed,
    Released,
    Held
};

/// @brief Enum representing the axis of a mouse.
enum class MouseAxis {
    X,
    Y
};

/// @brief Struct representing an input binding. SDL_Scancode, state and std::function containing the action to be performed.
struct InputBinding {
    SDL_Scancode scancode;
    InputActionState state;
    std::function<void(float)> action;
};

/// @brief Struct representing an axis binding. Axis and std::function containing the action to be performed.
struct MouseAxisBinding {
    MouseAxis axis;
    std::function<void(float)> action;
};

/// @brief Struct for Gamepad Button bindings
struct GamepadButtonBinding {
    SDL_GamepadButton button;
    InputActionState state;
    std::function<void(float)> action;
};

/// @brief Struct for Gamepad Axis bindings (Joysticks/Triggers)
struct GamepadAxisBinding {
    SDL_GamepadAxis axis;
    std::function<void(float)> action;
};


/// @brief Component you add to your GameObject to handle input, eg. add input bindings and mouse axis bindings.
struct InputComponent {
    std::vector<InputBinding> bindings;
    std::vector<MouseAxisBinding> mouseAxisBindings;
    std::vector<GamepadButtonBinding> gamepadButtonBindings;
    std::vector<GamepadAxisBinding> gamepadAxisBindings;

    /// @brief Adds input binding.
    /// @param SDL_Scancode scancode - The SDL_Scancode of the key to bind.
    /// @param InputActionState state - The state of the key to bind.
    /// @param std::function<void(float)> action - The action to perform when the key is pressed.
    ///
    /// @details Example usage:
    /// @code
    /// void onJump(float dt) {
    ///     player->jump(dt);
    /// }
    ///
    /// InputComponent* input = entity->addComponent<InputComponent>();
    /// input->addBinding(SDL_SCANCODE_SPACE, InputActionState::Pressed, onJump);
    /// @endcode
    void addBinding(SDL_Scancode scancode, InputActionState state, std::function<void(float)> action) {
        bindings.push_back({scancode, state, action});
    }

    /// @brief Adds axis binding.
    /// @param MouseAxis axis - The MouseAxis to bind.
    /// @param std::function<void(float)> action - The action to perform when the axis is moved.
    ///
    /// @details Example usage:
    /// @code
    /// void onMouseMove(float axis) {
    ///     player->GetComponent<vex::TransformComponent>().rotation.x += axis;
    /// }
    ///
    /// InputComponent* input = player->addComponent<InputComponent>();
    /// input->addMouseAxisBinding(MouseAxis::X, onMouseMove);
    /// @endcode
    void addMouseAxisBinding(MouseAxis axis, std::function<void(float)> action) {
        mouseAxisBindings.push_back({axis, action});
    }

    /// @brief Adds gamepad button binding.
    /// @param SDL_GamepadButton button - The SDL_GamepadButton to bind.
    /// @param InputActionState state - The state of the button to bind.
    /// @param std::function<void(float)> action - The action to perform when the button is pressed.
    ///
    /// @details Example usage:
    /// @code
    /// void onGamepadButtonPress(float dt) {
    ///     player->jump(dt);
    /// }
    ///
    /// InputComponent* input = entity->addComponent<InputComponent>();
    /// input->addGamepadButtonBinding(SDL_CONTROLLER_BUTTON_A, InputActionState::Pressed, onGamepadButtonPress);
    /// @endcode
    void addGamepadButtonBinding(SDL_GamepadButton button, InputActionState state, std::function<void(float)> action) {
        gamepadButtonBindings.push_back({button, state, action});
    }

    /// @brief Adds gamepad axis binding.
    /// @param SDL_GamepadAxis axis - The SDL_GamepadAxis to bind.
    /// @param std::function<void(float)> action - The action to perform when the axis is moved.
    ///
    /// @details Example usage:
    /// @code
    /// void onGamepadAxisMove(float axis) {
    ///     player->GetComponent<vex::TransformComponent>().rotation.x += axis;
    /// }
    ///
    /// InputComponent* input = player->addComponent<InputComponent>();
    /// input->addGamepadAxisBinding(SDL_CONTROLLER_AXIS_LEFTX, onGamepadAxisMove);
    /// @endcode
    void addGamepadAxisBinding(SDL_GamepadAxis axis, std::function<void(float)> action) {
        gamepadAxisBindings.push_back({axis, action});
    }

};

}
