// engine.cpp
#include "Engine.hpp"
#include "SDL3/SDL_events.h"

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

namespace vex {

Engine::Engine(const char* title, int width, int height, GameInfo gInfo) {
    std::filesystem::current_path(GetExecutableDir());
    m_gameInfo = gInfo;
    if(m_gameInfo.versionMajor == 0 && m_gameInfo.versionMinor == 0 && m_gameInfo.versionPatch == 0){
        log("Project version not set!");
    }

    m_window = std::make_unique<Window>(title, width, height);
    m_resolutionManager = std::make_unique<ResolutionManager>(m_window->GetSDLWindow());
    m_inputSystem = std::make_unique<InputSystem>(m_registry, m_window->GetSDLWindow());
    m_vfs = std::make_unique<VirtualFileSystem>();
    m_vfs->initialize(GetExecutableDir().string());

    m_physicsSystem = std::make_unique<PhysicsSystem>(m_registry);
    m_physicsSystem->init();

    m_sceneManager = std::make_unique<SceneManager>();

    auto renderRes = m_resolutionManager->getRenderResolution();
    log("Initializing Vulkan interface...");
    m_interface = std::make_unique<Interface>(m_window->GetSDLWindow(), renderRes, m_gameInfo, m_vfs.get());
    m_imgui = std::make_unique<VulkanImGUIWrapper>(m_window->GetSDLWindow(), *m_interface->getContext());
    m_imgui->init();
    //m_vexUI = std::make_unique<VexUI>(*m_interface->getContext(), m_vfs.get(), m_interface->getResources());
    //m_vexUI->init();

    log("Engine initialized successfully");
}


std::shared_ptr<SceneManager> Engine::getSceneManager() {
    return m_sceneManager;
}

std::shared_ptr<Interface> Engine::getInterface() {
    return m_interface;
}

std::shared_ptr<VexUI> Engine::createVexUI(){
    return std::make_unique<VexUI>(*m_interface->getContext(), m_vfs.get(), m_interface->getResources());
}

void Engine::run() {
    Uint64 lastTime = SDL_GetPerformanceCounter();

    while (m_running) {

        Uint64 now = SDL_GetPerformanceCounter();
        float deltaTime = (float)((now - lastTime) / (float)SDL_GetPerformanceFrequency());
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            m_inputSystem->processEvent(event, deltaTime);
            processEvent(event, deltaTime);
            m_imgui->processEvent(&event);
            //m_vexUI->processEvent(event);
            //
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
                    break;
                case SDL_EVENT_WILL_ENTER_BACKGROUND:
                    m_interface->unbindWindow();
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    m_resolutionManager->update();
                    auto renderRes = m_resolutionManager->getRenderResolution();
                    m_interface->setRenderResolution(renderRes);
                    break;
            }
        }

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

            if(!m_paused){
                update(deltaTime);
                m_sceneManager->scenesUpdate(deltaTime);
                m_physicsSystem->update(deltaTime);
            }
        }else{

            auto uiView = m_registry.view<UiComponent>();
            std::vector<UiComponent> uiObjects;
            for (auto entity : uiView) {
                if(!uiView.get<UiComponent>(entity).m_vexUI->isInitialized()){
                    //uiView.get<UiComponent>(entity).m_vexUI = std::make_unique<VexUI>(*m_interface->getContext(), m_vfs.get(), m_interface->getResources());
                    uiView.get<UiComponent>(entity).m_vexUI->init();
                    uiView.get<UiComponent>(entity).m_vexUI->update();
                }
            }

            beginGame();
        }

        render();
        m_frame++;
    }
}

Engine::~Engine() {
    m_imgui.reset();
    m_interface.reset();
    m_inputSystem.reset();
    m_window.reset();
    SDL_Quit();
}

void Engine::setResolutionMode(ResolutionMode mode) {
    m_resolutionManager->setMode(mode);
    m_resolutionManager->update();
    auto renderRes = m_resolutionManager->getRenderResolution();
    m_interface->setRenderResolution(renderRes);
}

void Engine::setEnvironmentSettings(enviroment settings) {
    m_interface->setEnvironment(settings);
}

enviroment Engine::getEnvironmentSettings() {
    return m_interface->getEnvironment();
}

void Engine::processEvent(const SDL_Event& event, float deltaTime) {}
void Engine::update(float deltaTime) {}
void Engine::beginGame() {}
void Engine::render() {
    //log("Render function called");
    auto renderRes = m_resolutionManager->getRenderResolution();

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);

    auto cameraEntity = getCamera();
    auto& transform = m_registry.get<TransformComponent>(cameraEntity);
    auto& camera = m_registry.get<CameraComponent>(cameraEntity);

    view = glm::lookAt(
        transform.getWorldPosition(m_registry),
        transform.getWorldPosition(m_registry) + transform.getForwardVector(m_registry),
        transform.getUpVector(m_registry)
    );
    proj = glm::perspective(
        glm::radians(camera.fov),
        renderRes.x / static_cast<float>(renderRes.y),
        camera.nearPlane,
        camera.farPlane
    );
    proj[1][1] *= -1;

    //log("Calling Renderer::renderFrame()");
    try{
        m_interface->getRenderer().renderFrame(view, proj, renderRes, m_registry, *m_imgui, m_frame);
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Frame not in fact rendered :C");
        handle_exception(e);
    }
    //log("Frame Rendered");
}

}
