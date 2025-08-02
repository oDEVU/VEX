#pragma once
#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <unordered_map>
#include "SDL3/SDL_mouse.h"
#include "components/GameComponents/InputComponent.hpp"


namespace vex {

enum class InputMode {
    Game,    // Mouse captured, hidden, for gameplay (e.g., camera control)
    UI,      // Mouse visible, for UI interaction
    GameAndUI // Mouse visible, both gameplay and UI inputs processed
};

class InputSystem {
public:
    InputSystem(entt::registry& registry, SDL_Window* window)
        : m_registry(registry), m_window(window), m_inputMode(InputMode::Game) {
        setInputMode(InputMode::Game);
    }

    void setInputMode(InputMode mode);

    InputMode getInputMode() const { return m_inputMode; }

    void processEvent(const SDL_Event& event, float deltaTime);

    void update(float deltaTime);

private:
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
