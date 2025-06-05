#pragma once

#include <cstdint>
#include <memory>
#include <SDL3/SDL.h>

#include "components/ResolutionManager.hpp"
#include "components/Model.hpp"
#include "components/Camera.hpp"
#include "components/GameInfo.hpp"
#include "components/ImGUIWrapper.hpp"

namespace vex {

class Window;
class Interface;

class Engine {
public:
    Engine(const char* title, int width, int height, GameInfo gInfo);
    ~Engine();

    void run();

    std::shared_ptr<Camera> getCamera() {return m_camera;}

    void setResolutionMode(ResolutionMode mode) { resolutionManager->setMode(mode); }
    ResolutionMode getResolutionMode() const { return resolutionManager->getCurrentMode(); }

    Model& loadModel(const std::string& modelPath, const std::string& name);
    Model* getModel(const std::string& name);

    virtual void processEvent(const SDL_Event& event, float deltaTime);
    virtual void beginGame();
    virtual void update(float deltaTime);
    virtual void render();
protected:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Interface> m_interface;
    std::unique_ptr<ResolutionManager> resolutionManager;
    std::unique_ptr<ImGUIWrapper> m_imgui;

    bool m_running = true;
    u_int64_t frame = 0;

    std::shared_ptr<Camera> m_camera;

    GameInfo gameInfo;
};
}
