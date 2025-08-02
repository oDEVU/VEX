// engine.cpp
#include "Engine.hpp"
#include "SDL3/SDL_events.h"

#include "components/Window.hpp"
#include "components/pathUtils.hpp"

#include "components/backends/vulkan/Interface.hpp"
#include "components/backends/vulkan/VulkanImGUIWrapper.hpp"
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

    // Testing resolution modes
    //m_resolutionManager->setMode(ResolutionMode::NATIVE);       // Native
    //m_resolutionManager->setMode(ResolutionMode::PS1_SHARP);    // Close to ps1 resolution but intiger scaled
    //m_resolutionManager->setMode(ResolutionMode::RES_240P);     // Height same as original but not intiger scaled (wierd scaling)
    //m_resolutionManager->setMode(ResolutionMode::RES_480P);     // Height same as original but not intiger scaled (wierd scaling)
    //m_resolutionManager->update();

    auto renderRes = m_resolutionManager->getRenderResolution();
    log("Initializing Vulkan interface...");
    m_interface = std::make_unique<Interface>(m_window->GetSDLWindow(), renderRes, m_gameInfo);
    m_imgui = std::make_unique<VulkanImGUIWrapper>(m_window->GetSDLWindow(), *m_interface->getContext());
    m_imgui->init();

    log("Engine initialized successfully");
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
            update(deltaTime);
        }else{
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

//entt::entity Engine::loadModel(const std::string& modelPath, const std::string& name, entt::entity parent){
//    return m_interface->getMeshManager().loadModel(modelPath, name, m_registry, parent);
//}

//Model* Engine::getModel(const std::string& name){
//    return m_interface->getMeshManager().getModel(name);
//}

void Engine::processEvent(const SDL_Event& event, float deltaTime) {}
void Engine::update(float deltaTime) {}
void Engine::beginGame() {}
void Engine::render() {
    auto renderRes = m_resolutionManager->getRenderResolution();

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);
    auto cameraEntity = getCamera();//m_registry.view<TransformComponent, CameraComponent>();
    //for (auto entity : cameraView) {
        auto& transform = m_registry.get<TransformComponent>(cameraEntity);
        auto& camera = m_registry.get<CameraComponent>(cameraEntity);
        view = glm::lookAt(
            transform.position,
            transform.position + transform.getForwardVector(),
            transform.getUpVector()
        );
        proj = glm::perspective(
            glm::radians(camera.fov),
            renderRes.x / static_cast<float>(renderRes.y),
            camera.nearPlane,
            camera.farPlane
        );
        proj[1][1] *= -1;
       // break;
       //}
    /*
    glm::mat4 view = glm::lookAt(m_camera->transform.position, glm::vec3(m_camera->transform.position + m_camera->transform.getForwardVector()), m_camera->transform.getUpVector());
    glm::mat4 proj = glm::perspective(
        glm::radians(m_camera->fov),
        renderRes.x / static_cast<float>(renderRes.y),
        m_camera->nearPlane,
        m_camera->farPlane
    );
    proj[1][1] *= -1;*/

    m_interface->getRenderer().renderFrame(view, proj, renderRes, m_registry, *m_imgui, m_frame);
}

}
