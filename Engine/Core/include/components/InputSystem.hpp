/**
 *  @file   InputSystem.hpp
 *  @brief  This file defines InputSystem class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <unordered_map>
#include "SDL3/SDL_mouse.h"
#include "components/GameComponents/InputComponent.hpp"
#include "components/errorUtils.hpp"


namespace vex {

    /// @brief Enum representing different input modes.
    /// @details Game - Mouse captured, hidden, for gameplay (e.g., camera control)
    /// @details UI - Mouse visible, for UI interaction
    /// @details GameAndUI - Mouse visible, both gameplay and UI inputs processed
enum class InputMode {
    Game,
    UI,
    GameAndUI
};

/// @brief Input system class responsible for handling input events and updating input components.
class InputSystem {
public:
    /// @brief Constructor for InputSystem.
    /// @param entt::registry& registry Reference to the entity registry.
    /// @param SDL_Window* window Pointer to the SDL window.
    InputSystem(entt::registry& registry, SDL_Window* window);

    /// @brief Sets the input processing mode.
    /// @details
    /// - **Game**: Captures the mouse (Relative Mode) and hides the cursor via `SDL_SetWindowRelativeMouseMode`.
    /// - **UI**: Releases the mouse and shows the cursor.
    /// @param InputMode mode - The desired InputMode.
    void setInputMode(InputMode mode);

    /// @brief Gets the current input mode.
    /// @return InputMode The current input mode.
    InputMode getInputMode() const { return m_inputMode; }

    /// @brief Processes raw SDL events and maps them to Game Component actions.
    /// @details Handles:
    /// - **Keyboard**: Updates internal `keyStates` and triggers `InputComponent` bindings (Pressed/Released/Held).
    /// - **Mouse**: Accumulates relative motion deltas for axis bindings.
    /// - **Gamepad**: Handles button presses and axis motion (normalized to 0.0-1.0).
    /// @param const SDL_Event& event - The raw SDL_Event to parse.
    /// @param float deltaTime - Time elapsed, passed to action callbacks.
    void processEvent(const SDL_Event& event, float deltaTime);

    /// @brief Per-frame update for continuous input states.
    /// @details Specifically checks for and triggers actions bound to `InputActionState::Held` for both Keyboard and Gamepad buttons.
    /// @param float deltaTime - Time elapsed since last frame.
    void update(float deltaTime);

private:
    /// @brief Key state structure. Used to track the state of keyboard keys needed forr held state.
    struct KeyState {
        bool isPressed = false;
        bool wasProcessedAsPressed = false;
    };

    entt::registry& m_registry;
    SDL_Window* m_window;
    InputMode m_inputMode;
    std::unordered_map<SDL_Scancode, KeyState> keyStates;
    std::unordered_map<SDL_GamepadButton, KeyState> gamepadButtonStates;
};

}
