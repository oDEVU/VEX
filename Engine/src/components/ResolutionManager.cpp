// resolution.cpp
#include "components/ResolutionManager.hpp"
#include "Window.hpp"

namespace vex {

ResolutionManager::ResolutionManager(SDL_Window* window) : m_p_window(window) {
    update();
}

void ResolutionManager::setMode(ResolutionMode mode) {
    m_currentMode = mode;
    update();
}

void ResolutionManager::update() {
    int width, height;
    SDL_GetWindowSizeInPixels(m_p_window, &width, &height);
    m_windowResolution = {width, height};

    switch(m_currentMode) {
        case ResolutionMode::NATIVE:
            m_renderResolution = m_windowResolution;
            m_upscaleRatio = 1.0f;
            break;

        case ResolutionMode::RES_240P: {
            float aspect = width / static_cast<float>(height);
            m_renderResolution.y = 240;
            m_renderResolution.x = static_cast<uint32_t>(240 * aspect);
            m_upscaleRatio = height / 240.0f;
            break;
        }

        case ResolutionMode::RES_480P: {
            float aspect = width / static_cast<float>(height);
            m_renderResolution.y = 480;
            m_renderResolution.x = static_cast<uint32_t>(480 * aspect);
            m_upscaleRatio = height / 480.0f;
            break;
        }

        case ResolutionMode::PS1_SHARP:
            calculatePS1SharpResolution();
            break;
    }

    log("Resolution mode updated: %dx%d (render) -> %dx%d (window), scale: %.2f",
           m_renderResolution.x, m_renderResolution.y,
           m_windowResolution.x, m_windowResolution.y,
           m_upscaleRatio);
}

void ResolutionManager::calculatePS1SharpResolution() {
    int yscale = floor(m_windowResolution.y / 240);
    int maxScale = std::max(1, yscale);

    maxScale = std::clamp(maxScale, 1, 4);

    m_renderResolution = m_windowResolution / static_cast<unsigned int>(maxScale);
    m_upscaleRatio = maxScale;

    if (m_renderResolution.y < 240) {
        float aspect = m_windowResolution.x / static_cast<float>(m_windowResolution.y);
        m_renderResolution.y = 240;
        m_renderResolution.x = static_cast<uint32_t>(240 * aspect);
        m_upscaleRatio = m_windowResolution.y / 240.0f;
    }
}

} // namespace vex
