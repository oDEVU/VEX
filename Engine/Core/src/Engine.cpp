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
#include "components/SceneManager.hpp"
#include "components/backends/vulkan/context.hpp"
#include "entt/entity/fwd.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>

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

    log("Initializing virtual file system..");
    m_vfs = std::make_shared<VirtualFileSystem>();
    m_vfs->initialize(GetExecutableDir().string());

    auto renderRes = m_resolutionManager->getRenderResolution();
    log("Initializing Vulkan interface...");
    m_interface = std::make_unique<Interface>(m_window->GetSDLWindow(), renderRes, m_gameInfo, m_vfs.get());
    m_imgui = std::make_unique<VulkanImGUIWrapper>(m_window->GetSDLWindow(), *m_interface->getContext());
    m_imgui->init();

    log("Initializing engine components...");

    m_inputSystem = std::make_unique<InputSystem>(m_registry, m_window->GetSDLWindow());
    m_physicsSystem = std::make_unique<PhysicsSystem>(m_registry);
    m_physicsSystem->init();

    m_sceneManager = std::make_unique<SceneManager>();

    getInterface()->getMeshManager().init(this);
    log("Engine initialized successfully");
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
        if (onUpdateLoop) onUpdateLoop();

        Uint64 now = SDL_GetPerformanceCounter();
        float deltaTime = (float)((now - lastTime) / (float)SDL_GetPerformanceFrequency());
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            processEvent(event, deltaTime);
            m_imgui->processEvent(&event);
            auto uiView = m_registry.view<UiComponent>();
            std::vector<UiComponent> uiObjects;
            for (auto entity : uiView) {
                if(uiView.get<UiComponent>(entity).m_vexUI->isInitialized()){
                    uiView.get<UiComponent>(entity).m_vexUI->processEvent(event);
                }
            }
            switch (event.type) {
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

        auto transformView = m_registry.view<TransformComponent>();
        for (auto entity : transformView) {
            if(!transformView.get<TransformComponent>(entity).isReady()){
                transformView.get<TransformComponent>(entity).setRegistry(m_registry);
            }
        }

        update(deltaTime);
    }
}

Engine::~Engine() {
    m_imgui.reset();
    m_interface.reset();
    m_inputSystem.reset();
    m_window.reset();
    SDL_Quit();
}

Engine::Engine(SkipInit) {
    std::cout << "Engine initialization handed of to Editor\n";
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

void Engine::prepareScenesForHotReload() {
   lastLoadedScenes = getSceneManager()->GetAllSceneNames();
}

void Engine::processEvent(const SDL_Event& event, float deltaTime) {
    m_inputSystem->processEvent(event, deltaTime);
}

void Engine::update(float deltaTime) {
    m_inputSystem->update(deltaTime);

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

        m_interface->getRenderer().renderScene(renderData, cameraEntity, m_registry, m_frame);
        m_interface->getRenderer().composeFrame(renderData, *m_imgui, false);
        m_interface->getRenderer().endFrame(renderData);
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Frame not in fact rendered :C");
        handle_exception(e);
    }
    //log("Frame Rendered");
}

}
