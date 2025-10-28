/**
 *  @file   Engine.hpp
 *  @brief  Contains main Engine class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <cstdint>
#include <memory>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <entt/entt.hpp>

#include "components/ResolutionManager.hpp"
#include "components/GameInfo.hpp"
#include "components/ImGUIWrapper.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/InputSystem.hpp"
#include "components/enviroment.hpp"
#include "components/UI/VexUI.hpp"

namespace vex {

class Window;
class Interface;
class VirtualFileSystem;

/// @brief Class for interaction with engine systems, your game main class will Inherit from it.
class Engine {
public:
    /// @brief Constructor for Engine class, that will handle initialization of every system with given params.
    /// @param char* title       - Title of the game window.
    /// @param int width  - Initial width of the game window.
    /// @param int length - Initial height of the game window.
    /// @param GameInfo gInfo    - Game Info class containing version, name etc.
    ///
    Engine(const char* title, int width, int height, GameInfo gInfo);
    ~Engine();

    /// @brief Function starting and running main game loop.
    void run();

    entt::entity getCamera() {
        auto view = m_registry.view<CameraComponent>();
        return view.empty() ? entt::null : view.front();
    }

    /// @brief Function allowing for changing resolution mode at runtime.
    /// @param ResolutionMode mode - Enum storing resolution modes.
    void setResolutionMode(ResolutionMode mode);

    /// @brief Returns the current resolution mode.
    /// @return ResolutionMode - Current resolution mode.
    ResolutionMode getResolutionMode() const { return m_resolutionManager->getCurrentMode(); }

    /// @brief Function allowing for changing input mode at runtime.
    /// @param InputMode mode - Enum storing input modes.
    void setInputMode(InputMode mode) { m_inputSystem->setInputMode(mode); }

    /// @brief Returns the current input mode.
    /// @return InputMode - Current input mode.
    InputMode getInputMode() const { return m_inputSystem->getInputMode(); }

    /// @brief Function allowing for changing environment settings at runtime.
    /// @param enviroment settings - Enum storing environment settings.
    void setEnvironmentSettings(enviroment settings);

    /// @brief Returns the current environment settings.
    /// @return enviroment - Current environment settings.
    enviroment getEnvironmentSettings();

    std::shared_ptr<Interface> getInterface();// { return m_interface; }
    std::shared_ptr<VirtualFileSystem> getFileSystem() { return m_vfs; }
    entt::registry& getRegistry() { return m_registry; }
    std::shared_ptr<VexUI> createVexUI();// { return std::make_unique<VexUI>(m_interface->getContext(), m_vfs.get(), m_interface->getResources()); }

    /// @brief Internal virtual function for handling window/keyboard events.
    /// @param SDL_Event - Event to process.
    /// @param float - Time since last frame.
    virtual void processEvent(const SDL_Event& event, float deltaTime);

    /// @brief Internal virtual function for handling stuff right at the beginning of the game but after initialization of the engine.
    virtual void beginGame();

    /// @brief Internal virtual function for handling updates, it is called every frame rightbefore rendering.
    /// @param float deltaTime - Time since last frame.
    virtual void update(float deltaTime);

    /// @brief Internal virtual function calling renderers renderFrame function. It also handles camera matrixes since camera object is handled by engine anyway and theres no reason to mass it to renderer.
    /// @see vex::Renderer::renderFrame()
    /// @param float deltaTime - Time since last frame.
    virtual void render();
protected:
    std::shared_ptr<Window> m_window;
    std::shared_ptr<Interface> m_interface;
    std::shared_ptr<ResolutionManager> m_resolutionManager;
    std::shared_ptr<ImGUIWrapper> m_imgui;
    std::unique_ptr<InputSystem> m_inputSystem;
    std::shared_ptr<VirtualFileSystem> m_vfs;
    //std::shared_ptr<VexUI> m_vexUI;

    entt::registry m_registry;

    bool m_running = true;
    int m_frame = 0;

    GameInfo m_gameInfo;
};
}
