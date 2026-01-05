/**
 *  @file   Engine.hpp
 *  @brief  Contains main Engine class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#if defined(_WIN32)
  #define NOMINMAX
#endif

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
#include "components/PhysicsSystem.hpp"

#include "VEX/VEX_export.h"

namespace vex {

class Window;
class Interface;
class VirtualFileSystem;
class SceneManager;

struct SkipInit {};

/// @brief Class for interaction with engine systems.
class VEX_EXPORT Engine {
public:
    /// @brief returns engine hash generated during build process.
    static const char* GetBuildHash();

    /// @brief Constructor for the Engine class.
    /// @details Initializes the core systems in the following order:
    /// 1. Window creation and ResolutionManager.
    /// 2. Virtual File System (VFS) rooted at the executable directory.
    /// 3. PhysicsSystem.
    /// 4. Vulkan Interface (Renderer) and ImGui wrapper.
    /// 5. InputSystem and SceneManager.
    /// @note Detects Wayland on Linux to enforce software VSync strategies if necessary.
    /// @param const char* title - Title of the game window.
    /// @param int width - Initial width of the game window.
    /// @param int height - Initial height of the game window.
    /// @param GameInfo gInfo - Game Info struct containing versioning and metadata.
    Engine(const char* title, int width, int height, GameInfo gInfo);
    ~Engine();

    /// @brief Starts and runs the main game loop.
    /// @details Handles the lifecycle of the application, including:
    /// - Event polling (`SDL_PollEvent`) and dispatching.
    /// - Frame timing, delta time calculation, and frame limiting (FPS cap).
    /// - Handling application background/foreground states (`SDL_EVENT_WILL_ENTER_BACKGROUND`) to pause/resume rendering.
    /// - Triggering Update and Render cycles.
    /// @param std::function<void()> onUpdateLoop - Optional callback function, primarily used for hot-reload logic to inject code into the loop.
    void run(std::function<void()> onUpdateLoop = nullptr);

    /// @brief Function returning the entity of the camera.
    /// @return entt::entity - Entity of the camera.
    entt::entity getCamera() {
        auto view = m_registry.view<CameraComponent>();
        return view.empty() ? entt::null : view.front();
    }

    /// @brief Sets the resolution mode and updates the resolution manager.
    /// @param ResolutionMode mode - The new resolution mode (e.g., NATIVE, PS1_SHARP).
    void setResolutionMode(ResolutionMode mode);

    /// @brief Internal function for handling pausing and resuming the game.
    /// @param bool paused - Whether the game should be paused or resumed.
    void setPaused(bool paused) { m_paused = paused; }

    /// @brief Function returning the current state of the game.
    /// @return bool - Whether the game is paused or not.
    bool isPaused() const { return m_paused; }

    /// @brief Returns the current resolution mode.
    /// @return ResolutionMode - Current resolution mode.
    ResolutionMode getResolutionMode() const { return m_resolutionManager->getCurrentMode(); }

    /// @brief Function allowing for changing input mode at runtime.
    /// @param InputMode mode - Enum storing input modes.
    void setInputMode(InputMode mode) { m_inputSystem->setInputMode(mode); }

    /// @brief Returns the current input mode.
    /// @return InputMode - Current input mode.
    InputMode getInputMode() const { return m_inputSystem->getInputMode(); }

    /// @brief Applies new global environment settings (lighting, shading) to the interface.
    /// @param enviroment settings - The new environment configuration struct.
    void setEnvironmentSettings(enviroment settings);

    /// @brief Returns the current environment settings.
    /// @return enviroment - Current environment settings.
    enviroment getEnvironmentSettings();

    /// @brief Returns Interface, used internally.
    Interface* getInterface();

    /// @brief Returns  std::shared_ptr of VirtualFileSystem.
    std::shared_ptr<VirtualFileSystem> getFileSystem() { return m_vfs; }

    /// @brief Returns a reference to entt::registry.
    entt::registry& getRegistry() { return m_registry; }

    /// @brief Returns pointer to PhysicsSystem.
    PhysicsSystem* getPhysicsSystem() { return m_physicsSystem.get(); }

    /// @brief Returns pointer to SceneManager.
    SceneManager* getSceneManager();

    /// @brief Creates and returns a shared pointer to a VexUI instance.
    /// @details Constructs a VexUI object linked to the current Vulkan context, VFS, and resource manager.
    /// @return std::shared_ptr<VexUI> - A ready-to-use UI instance.
    std::shared_ptr<VexUI> createVexUI();

    /// @brief Returns current frame number.
    int GetCurrentFrame() { return m_frame; }

    /// @brief Function to enable/disable rendering collision shapes.
    /// @param bool value - True to enable, false to disable.
    void setRenderPhysicsDebug(bool value) { m_renderPhysicsDebug = value; }

    /// @brief Function to enable/disable fullscreen mode. with optional exclusive parameter.
    /// @param bool enabled - True to enable, false to disable.
    /// @param bool exclusive - True to enable exclusive fullscreen mode, false to disable.
    virtual void setFullscreen(bool enabled, bool exclusive = false);

    /// @brief Function to check if fullscreen mode is enabled.
    bool isFullscreen();

    /// @brief Function to get the last loaded scenes.
    std::vector<std::string> getLastLoadedScenes() { return lastLoadedScenes; }

    /// @brief Prepares the engine for a hot-reload event.
    /// @details Snapshots the names of currently loaded scenes into `lastLoadedScenes` via `SceneManager::GetAllSceneNames` so they can be restored after the reload.
    void prepareScenesForHotReload();

    /// @brief Internal virtual function for handling window/keyboard events.
    /// @details Delegates processing to the `InputSystem`.
    /// @param const SDL_Event& event - The SDL event to process.
    /// @param float deltaTime - Time since last frame.
    virtual void processEvent(const SDL_Event& event, float deltaTime);

    /// @brief Internal virtual function for handling stuff right at the beginning of the game but after initialization of the engine.
    virtual void beginGame();

    /// @brief Internal function for handling updates; called every frame before rendering.
    /// @details Updates the InputSystem, initializes/updates dynamic UiComponents, and steps the SceneManager and PhysicsSystem.
    /// Skips scene and physics updates if the engine is paused (`m_paused` or `m_internally_paused`).
    /// @param float deltaTime - Time elapsed since the last frame in seconds.
    virtual void update(float deltaTime);

    /// @brief Internal function that orchestrates the frame rendering process.
    /// @details
    /// 1. Acquires the current render resolution and Camera entity.
    /// 2. Prepares `SceneRenderData` via the Vulkan Interface.
    /// 3. If `m_renderPhysicsDebug` is enabled, populates debug lines for wireframe rendering.
    /// 4. Calls `Renderer::renderScene` followed by UI composition and frame presentation.
    /// @see vex::Renderer
    /// @throws std::exception - If frame rendering fails, logging the error before handling.
    virtual void render();

    /// @brief Internal virtual function for editor.
    virtual void refreshForObject() {
        log("Editor use only");
    }

    /// @brief Simply sets m_running to false, Effectivelly soft shuting down the engine.
    void quit(){ m_running = false; }

    /// @brief Waits for the GPU to finish all pending operations.
    /// @details Essential to call before destroying resources (e.g., during shutdown or scene transitions) to prevent Vulkan validation errors or crashes.
    void WaitForGpu();

    /// @brief Sets the target frame rate limit.
    /// @details Uses `SDL_Delay` in the main loop to sleep if the frame completes faster than the target time.
    /// @param int fps - Target FPS. Pass 0 to disable the limit (unlimited FPS).
    void setFrameLimit(int fps);
    int getFrameLimit() const { return m_targetFps; }

    /// @brief Toggles VSync (Vertical Synchronization).
    void setVSync(bool enabled);

    /// @brief Returns the current state of VSync.
    bool getVSync() const;

    /// @brief Returns the time since last frame.
    float getDeltaTime() const { return m_deltaTime; }
protected:

    /// @brief Alternative constructor for editor to skip default init code.
    Engine(SkipInit);

    std::shared_ptr<Window> m_window;
    std::shared_ptr<VirtualFileSystem> m_vfs;

    std::unique_ptr<Interface> m_interface;
    std::unique_ptr<ResolutionManager> m_resolutionManager;
    std::unique_ptr<ImGUIWrapper> m_imgui;
    std::unique_ptr<InputSystem> m_inputSystem;
    std::unique_ptr<PhysicsSystem> m_physicsSystem;
    std::unique_ptr<SceneManager> m_sceneManager;

    entt::registry m_registry;

    bool m_running = true;
    bool m_paused = false;
    bool m_internally_paused = false; // to pause if the window is in the background.
    bool m_renderPhysicsDebug = false;
    int m_frame = 0;

    int m_targetFps = 0;
    bool m_vsyncEnabled = false;

    float m_deltaTime = 0.016f;

    #ifdef __linux__
        bool m_isWayland = false;
    #endif

    GameInfo m_gameInfo;
    std::vector<std::string> lastLoadedScenes;
};
}
