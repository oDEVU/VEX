#pragma once

#include <memory>
#include <SDL3/SDL.h>

#include "window.hpp"
#include "backends/vulkan/interface.hpp"

namespace vex {

class Engine {
public:
    Engine(const char* title, int width, int height);
    ~Engine();

    // Main loop control
    void run();

    // Accessors
    SDL_Window* getWindow() const;
    Interface* getInterface() const;

    // Event handling hooks
    virtual void processEvent(const SDL_Event& event);
    virtual void update(float deltaTime);
    virtual void render();

protected:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Interface> m_interface;
    bool m_running = true;
    u_int64_t frame = 0;
};
}
