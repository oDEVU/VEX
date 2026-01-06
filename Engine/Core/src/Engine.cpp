// engine.cpp
#include "Engine.hpp"
#include "SDL3/SDL_events.h"

#include "components/GameComponents/BasicComponents.hpp"
#include "components/VirtualFileSystem.hpp"
#include "components/Window.hpp"
#include "components/pathUtils.hpp"
#include <components/GameComponents/UiComponent.hpp>

#include "components/backends/vulkan/Interface.hpp"
#include "components/backends/vulkan/VulkanImGUIWrapper.hpp"
#include "components/backends/vulkan/PhysicsDebug.hpp"

#include "components/SceneManager.hpp"
#include "components/backends/vulkan/context.hpp"
#include "entt/entity/fwd.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>

#include "VexBuildVersion.hpp"

namespace vex {

Engine::Engine(const char* title, int width, int height, GameInfo gInfo) {
    std::filesystem::current_path(GetExecutableDir());
    m_gameInfo = gInfo;
    if(m_gameInfo.versionMajor == 0 && m_gameInfo.versionMinor == 0 && m_gameInfo.versionPatch == 0){
        log("Project version not set!");
    }

    log("Creating window..");
    m_window = std::make_shared<Window>(title, width, height);
    m_resolutionManager = std::make_unique<ResolutionManager>(m_window->GetSDLWindow());

    #ifdef __linux__
    const char* driver = SDL_GetCurrentVideoDriver();
    if (driver && std::string(driver) == "wayland") {
        m_isWayland = true;
        log("Wayland detected: Enforcing Software VSync strategy.");
    }
    #endif

    log("Initializing virtual file system..");
    m_vfs = std::make_shared<VirtualFileSystem>();
    m_vfs->initialize(GetExecutableDir().string());

    m_physicsSystem = std::make_unique<PhysicsSystem>(m_registry);
    m_physicsSystem->init();

    m_audioSystem = std::make_unique<AudioSystem>(m_registry);
    m_audioSystem->Init(m_vfs.get());

    auto renderRes = m_resolutionManager->getRenderResolution();
    log("Initializing Vulkan interface...");
    m_interface = std::make_unique<Interface>(m_window->GetSDLWindow(), renderRes, m_gameInfo, m_vfs.get());
    m_imgui = std::make_unique<VulkanImGUIWrapper>(m_window->GetSDLWindow(), *m_interface->getContext());
    m_imgui->init();

    log("Initializing engine components...");

    m_inputSystem = std::make_unique<InputSystem>(m_registry, m_window->GetSDLWindow());
    m_sceneManager = std::make_unique<SceneManager>();

    getInterface()->getMeshManager().init(this);
    log("Engine initialized successfully");
}

const char* Engine::GetBuildHash() {
        return VEX_ENGINE_BUILD_ID;
    }

SceneManager* Engine::getSceneManager() {
    return m_sceneManager.get();
}

Interface* Engine::getInterface() {
    return m_interface.get();
}

std::shared_ptr<VexUI> Engine::createVexUI(){
    return std::make_shared<VexUI>(*m_interface->getContext(), m_vfs.get(), m_interface->getResources());
}

void Engine::run(std::function<void()> onUpdateLoop) {
    Uint64 lastTime = SDL_GetPerformanceCounter();

    while (m_running) {
        Uint64 frameStart = SDL_GetPerformanceCounter();
        if (onUpdateLoop) onUpdateLoop();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            processEvent(event, m_deltaTime);
            m_imgui->processEvent(&event);
            auto uiView = m_registry.view<UiComponent>();
            std::vector<UiComponent> uiObjects;
            for (auto entity : uiView) {
                if(uiView.get<UiComponent>(entity).m_vexUI->isInitialized()){
                    uiView.get<UiComponent>(entity).m_vexUI->processEvent(event);
                }
            }
            switch (event.type) {
                case SDL_EVENT_GAMEPAD_ADDED:
                    SDL_OpenGamepad(event.gdevice.which);
                    log("Gamepad connected and opened.");
                    break;
                case SDL_EVENT_QUIT:
                    m_running = false;
                    break;
                case SDL_EVENT_DID_ENTER_FOREGROUND:
                    log("Binding window to Vulkan...");
                    m_interface->bindWindow(m_window->GetSDLWindow());
                    m_internally_paused = false;
                    break;
                case SDL_EVENT_WILL_ENTER_BACKGROUND:
                    m_interface->unbindWindow();
                    m_internally_paused = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    m_resolutionManager->update();
                    //auto renderRes = m_resolutionManager->getRenderResolution();
                    //m_interface->setRenderResolution(renderRes);
                    break;
            }
        }

        if (!m_running) break;

        auto transformView = m_registry.view<TransformComponent>();
        for (auto entity : transformView) {
            if(!transformView.get<TransformComponent>(entity).isReady()){
                transformView.get<TransformComponent>(entity).setRegistry(m_registry);
            }
        }

        update(m_deltaTime);

        float targetFps = (float)m_targetFps;

        #ifdef __linux__
            if (m_vsyncEnabled && m_isWayland) {
                targetFps = m_window->getRefreshRate();
            }
        #endif

        if (targetFps > 0) {
            float targetFrameTime = 1000.0f / targetFps;

            Uint64 frameEnd = SDL_GetPerformanceCounter();
            float elapsedMS = (frameEnd - frameStart) / (float)SDL_GetPerformanceFrequency() * 1000.0f;

            if (elapsedMS < targetFrameTime) {
                SDL_Delay(static_cast<Uint32>(targetFrameTime - elapsedMS));
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        m_deltaTime = (float)((now - lastTime) / (float)SDL_GetPerformanceFrequency());
        lastTime = now;
    }

    if (m_interface) {
        log("Engine shutdown requested. Waiting for GPU to finish...");
        m_interface->WaitForGPUToFinish();
    }
}

void Engine::WaitForGpu() {
    if (m_interface) {
        m_interface->WaitForGPUToFinish();
    }
}

void Engine::setFrameLimit(int fps) {
    m_targetFps = fps;
}

void Engine::setVSync(bool enabled) {
    if (m_vsyncEnabled == enabled) return;
    m_vsyncEnabled = enabled;

    if (m_interface) {
        m_interface->setVSync(enabled);
    }
}

bool Engine::getVSync() const {
    return m_vsyncEnabled;
}

Engine::~Engine() {
    m_audioSystem->Shutdown();
    m_audioSystem.reset();
    if (m_physicsSystem) {
        m_physicsSystem->shutdown();
        m_physicsSystem.reset();
    }
    m_imgui.reset();
    m_interface.reset();
    m_inputSystem.reset();
    m_window.reset();
    SDL_Quit();
}

Engine::Engine(SkipInit) {
    log("Engine initialization handed of to Editor");
}

void Engine::setResolutionMode(ResolutionMode mode) {
    m_resolutionManager->setMode(mode);
    m_resolutionManager->update();
    //auto renderRes = m_resolutionManager->getRenderResolution();
    //m_interface->setRenderResolution(renderRes);
}

void Engine::setEnvironmentSettings(enviroment settings) {
    m_interface->setEnvironment(settings);
}

enviroment Engine::getEnvironmentSettings() {
    return m_interface->getEnvironment();
}

void Engine::setFullscreen(bool enabled, bool exclusive) {
    if (m_window) {
        m_window->setFullscreen(enabled, exclusive);
    }
}

bool Engine::isFullscreen() {
    return m_window ? m_window->isFullscreen() : false;
}

void Engine::prepareScenesForHotReload() {
   lastLoadedScenes = getSceneManager()->GetAllSceneNames();
}

void Engine::processEvent(const SDL_Event& event, float deltaTime) {
    m_inputSystem->processEvent(event, deltaTime);
}

void Engine::update(float deltaTime) {
    m_inputSystem->update(deltaTime);

    auto cameraEntity = getCamera();
    if (cameraEntity != entt::null) {
        m_audioSystem->Update(cameraEntity);
    }

    if(m_frame > 0){

        auto uiView = m_registry.view<UiComponent>();
        std::vector<UiComponent> uiObjects;
        for (auto entity : uiView) {
            if(!uiView.get<UiComponent>(entity).m_vexUI->isInitialized()){
                //uiView.get<UiComponent>(entity).m_vexUI = std::make_unique<VexUI>(*m_interface->getContext(), m_vfs.get(), m_interface->getResources());
                uiView.get<UiComponent>(entity).m_vexUI->init();
                uiView.get<UiComponent>(entity).m_vexUI->update();
            }
        }

        if(!(m_paused || m_internally_paused)){
            m_sceneManager->scenesUpdate(deltaTime);
            m_physicsSystem->update(deltaTime);
        }
    }else{

        auto uiView = m_registry.view<UiComponent>();
        std::vector<UiComponent> uiObjects;
        for (auto entity : uiView) {
            if(!uiView.get<UiComponent>(entity).m_vexUI->isInitialized()){
                uiView.get<UiComponent>(entity).m_vexUI->init();
                uiView.get<UiComponent>(entity).m_vexUI->update();
            }
        }

        beginGame();
    }

    if(!m_internally_paused){
        render();
        m_frame++;
    }
}

void Engine::beginGame() {}
void Engine::render() {
    //log("Render function called");
    auto renderRes = m_resolutionManager->getRenderResolution();
    auto cameraEntity = getCamera();

    if (cameraEntity == entt::null) {
        return;
    }

    //std::cout << "renderRes x:" << renderRes.x << ", y:" << renderRes.y << std::endl;

    //log("Calling Renderer::renderFrame()");
    try{
        vex::SceneRenderData renderData;

        if (!m_interface->getRenderer().beginFrame(renderRes, renderData)) {
            return;
        }

        #if DEBUG
                const std::vector<DebugVertex>* debugLines = nullptr;
                if(m_renderPhysicsDebug) {
                     auto* dbg = m_interface->getPhysicsDebug();
                     dbg->Clear();
                     m_physicsSystem->setDebugRenderer(dbg);
                     m_physicsSystem->drawDebug();
                     debugLines = &dbg->GetLines();
                }
                m_interface->getRenderer().renderScene(renderData, cameraEntity, m_registry, m_frame, debugLines);
        #else
                m_interface->getRenderer().renderScene(renderData, cameraEntity, m_registry, m_frame);
        #endif

        m_interface->getRenderer().composeFrame(renderData, *m_imgui, false);
        m_interface->getRenderer().endFrame(renderData);
    } catch (const std::exception& e) {
        log(LogLevel::ERROR, "Frame render failed");
        handle_exception(e);
    }
    //log("Frame Rendered");
}

}
