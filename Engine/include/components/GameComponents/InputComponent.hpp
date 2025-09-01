#pragma once
#include <SDL3/SDL.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vex {

enum class InputActionState {
    Pressed,
    Released,
    Held
};

enum class MouseAxis {
    X,
    Y
};

struct InputBinding {
    SDL_Scancode scancode;
    InputActionState state;
    std::function<void(float)> action;
};

struct MouseAxisBinding {
    MouseAxis axis;
    std::function<void(float)> action;
};

struct InputComponent {
    std::vector<InputBinding> bindings;
    std::vector<MouseAxisBinding> mouseAxisBindings;

    void addBinding(SDL_Scancode scancode, InputActionState state, std::function<void(float)> action) {
        bindings.push_back({scancode, state, action});
    }

    void addMouseAxisBinding(MouseAxis axis, std::function<void(float)> action) {
        mouseAxisBindings.push_back({axis, action});
    }
};

}
