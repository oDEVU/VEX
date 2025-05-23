// engine.cpp
#include "engine.hpp"
#include "SDL3/SDL_events.h"

namespace vex {

Engine::Engine(const char* title, int width, int height) {

    // Create window
    m_window = std::make_unique<Window>(title, width, height);

    // Create resolution manager
    resolutionManager = std::make_unique<ResolutionManager>(m_window->GetSDLWindow());

    // Testing resolution modes
    //resolutionManager->setMode(ResolutionMode::NATIVE);       // Native
    //resolutionManager->setMode(ResolutionMode::PS1_SHARP);    // Close to original intiger scaled
    //resolutionManager->setMode(ResolutionMode::RES_240P);     // Height same as original but not intiger scaled (wierd scaling)
    //resolutionManager->setMode(ResolutionMode::RES_480P);     // Height same as original but not intiger scaled (wierd scaling)
    //resolutionManager->update();

    // Create renderer (could be Vulkan or OpenGL)
    //
    // Pass initial render resolution to interface
    auto renderRes = resolutionManager->getRenderResolution();
    SDL_Log("Initializing Vulkan interface...");
    m_interface = std::make_unique<Interface>(m_window->GetSDLWindow(), renderRes);


    SDL_Log("Engine initialized successfully");
}

void Engine::run() {
    Uint64 lastTime = SDL_GetPerformanceCounter();

    while (m_running) {
        // Event processing
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            processEvent(event);
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    m_running = false;
                    break;
                case SDL_EVENT_DID_ENTER_FOREGROUND:
                    SDL_Log("Binding window to Vulkan...");
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

        // Timing
        Uint64 now = SDL_GetPerformanceCounter();
        float deltaTime = (float)((now - lastTime) / (float)SDL_GetPerformanceFrequency());
        lastTime = now;

        // Update and render

        if(frame > 0){
            update(deltaTime);
        }

        render();
        frame++;
    }
}

Engine::~Engine() {
    // Cleanup resources in reverse order of initialization
    m_interface.reset();
    m_window.reset();
    SDL_Quit();
}

// Default implementations (can be overridden)
void Engine::processEvent(const SDL_Event& event) {}
void Engine::update(float deltaTime) {}
void Engine::render() {
    auto renderRes = resolutionManager->getRenderResolution();

    glm::mat4 view = glm::lookAt(glm::vec3(0,0,5), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        renderRes.x / static_cast<float>(renderRes.y),
        0.1f,
        100.0f
    );
    proj[1][1] *= -1;

    m_interface->renderFrame(view, proj, renderRes);
}

}
