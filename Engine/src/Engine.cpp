// engine.cpp
#include "Engine.hpp"
#include "SDL3/SDL_events.h"

#include "components/Window.hpp"

#include "components/backends/vulkan/Interface.hpp"
#include "components/backends/vulkan/VulkanImGUIWrapper.hpp"
#include "components/backends/vulkan/context.hpp"
#include <cstdint>

namespace vex {

Engine::Engine(const char* title, int width, int height, GameInfo gInfo) {
    gameInfo = gInfo;
    if(gameInfo.versionMajor == 0 && gameInfo.versionMinor == 0 && gameInfo.versionPatch == 0){
        log("Project version not set!");
    }

    m_window = std::make_unique<Window>(title, width, height);
    resolutionManager = std::make_unique<ResolutionManager>(m_window->GetSDLWindow());

    // Testing resolution modes
    //resolutionManager->setMode(ResolutionMode::NATIVE);       // Native
    //resolutionManager->setMode(ResolutionMode::PS1_SHARP);    // Close to ps1 resolution but intiger scaled
    //resolutionManager->setMode(ResolutionMode::RES_240P);     // Height same as original but not intiger scaled (wierd scaling)
    //resolutionManager->setMode(ResolutionMode::RES_480P);     // Height same as original but not intiger scaled (wierd scaling)
    //resolutionManager->update();

    auto renderRes = resolutionManager->getRenderResolution();
    log("Initializing Vulkan interface...");
    m_interface = std::make_unique<Interface>(m_window->GetSDLWindow(), renderRes, gameInfo);
    m_camera = std::make_unique<Camera>();
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
                    resolutionManager->update();
                    auto renderRes = resolutionManager->getRenderResolution();
                    m_interface->setRenderResolution(renderRes);
                    break;
            }
        }


        if(frame > 0){
            update(deltaTime);
        }else{
            beginGame();
        }

        render();
        frame++;
    }
}

Engine::~Engine() {
    m_interface.reset();
    m_window.reset();
    SDL_Quit();
}


Model& Engine::loadModel(const std::string& modelPath, const std::string& name){
    return m_interface->loadModel(modelPath, name);
}

Model* Engine::getModel(const std::string& name){
    return m_interface->getModel(name);
}

void Engine::processEvent(const SDL_Event& event, float deltaTime) {}
void Engine::update(float deltaTime) {}
void Engine::beginGame() {}
void Engine::render() {
    auto renderRes = resolutionManager->getRenderResolution();

    glm::mat4 view = glm::lookAt(m_camera->transform.position, glm::vec3(m_camera->transform.position + m_camera->transform.getForwardVector()), m_camera->transform.getUpVector());
    glm::mat4 proj = glm::perspective(
        glm::radians(m_camera->fov),
        renderRes.x / static_cast<float>(renderRes.y),
        m_camera->nearPlane,
        m_camera->farPlane
    );
    proj[1][1] *= -1;

    m_interface->renderFrame(view, proj, renderRes, *m_imgui, frame);
}

}
