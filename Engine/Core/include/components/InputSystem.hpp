/**
 *  @file   InputSystem.hpp
 *  @brief  This file defines InputSystem class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <unordered_map>
#include "SDL3/SDL_mouse.h"
#include "components/GameComponents/InputComponent.hpp"


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
    InputSystem(entt::registry& registry, SDL_Window* window)
        : m_registry(registry), m_window(window), m_inputMode(InputMode::Game) {
        setInputMode(InputMode::Game);
    }

    /// @brief Sets the input mode.
    /// @param InputMode mode The input mode to set.
    void setInputMode(InputMode mode);

    /// @brief Gets the current input mode.
    /// @return InputMode The current input mode.
    InputMode getInputMode() const { return m_inputMode; }

    /// @brief Processes an SDL event.
    /// @param const SDL_Event& event The SDL event to process.
    /// @param float deltaTime The time elapsed since the last update.
    void processEvent(const SDL_Event& event, float deltaTime);

    /// @brief Updates the input system.
    /// @param float deltaTime The time elapsed since the last update.
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
};

}
