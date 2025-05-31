#pragma once

#include <memory>
#include <SDL3/SDL.h>

#include "components/ResolutionManager.hpp"
#include "components/Model.hpp"

namespace vex {

class Window;
class Interface;

class Engine {
public:
    Engine(const char* title, int width, int height);
    ~Engine();

    void run();

    SDL_Window* getWindow() const;
    Interface* getInterface() const;

    void setResolutionMode(ResolutionMode mode) { resolutionManager->setMode(mode); }
    ResolutionMode getResolutionMode() const { return resolutionManager->getCurrentMode(); }

    Model& loadModel(const std::string& modelPath, const std::string& name);
    Model* getModel(const std::string& name);

    virtual void processEvent(const SDL_Event& event);
    virtual void update(float deltaTime);
    virtual void render();

protected:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Interface> m_interface;
    std::unique_ptr<ResolutionManager> resolutionManager;
    bool m_running = true;
    u_int64_t frame = 0;
};
}
