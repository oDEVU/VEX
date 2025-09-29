#pragma once

#include <cstdint>
#include <memory>
#include <SDL3/SDL.h>
#include <entt/entt.hpp>

#include "components/ResolutionManager.hpp"
#include "components/GameInfo.hpp"
#include "components/ImGUIWrapper.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/InputSystem.hpp"

namespace vex {

class Window;
class Interface;
class VirtualFileSystem;

class Engine {
public:
    Engine(const char* title, int width, int height, GameInfo gInfo);
    ~Engine();

    void run();

    entt::entity getCamera() {
        auto view = m_registry.view<CameraComponent>();
        return view.empty() ? entt::null : view.front();
    }

    void setResolutionMode(ResolutionMode mode);
    ResolutionMode getResolutionMode() const { return m_resolutionManager->getCurrentMode(); }
    void setInputMode(InputMode mode) { m_inputSystem->setInputMode(mode); }
    InputMode getInputMode() const { return m_inputSystem->getInputMode(); }

    //entt::entity loadModel(const std::string& modelPath, const std::string& name, entt::entity parent = entt::null);
    //Model* getModel(const std::string& name);

    std::shared_ptr<Interface> getInterface() { return m_interface; }
    std::shared_ptr<VirtualFileSystem> getFileSystem() { return m_vfs; }
    entt::registry& getRegistry() { return m_registry; }

    virtual void processEvent(const SDL_Event& event, float deltaTime);
    virtual void beginGame();
    virtual void update(float deltaTime);
    virtual void render();
protected:
    std::shared_ptr<Window> m_window;
    std::shared_ptr<Interface> m_interface;
    std::shared_ptr<ResolutionManager> m_resolutionManager;
    std::shared_ptr<ImGUIWrapper> m_imgui;
    std::unique_ptr<InputSystem> m_inputSystem;
    std::shared_ptr<VirtualFileSystem> m_vfs;

    entt::registry m_registry;

    bool m_running = true;
    u_int64_t m_frame = 0;

    GameInfo m_gameInfo;
};
}
