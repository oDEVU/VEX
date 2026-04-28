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

    static constexpr uint32_t MIN_DIMENSION = 64;
    static constexpr uint32_t MAX_DIMENSION = 32768;

    if (width <= 0 || height <= 0) {
        log(LogLevel::WARNING, "ResolutionManager::update() called with invalid window dimensions: %dx%d. Skipping update.",
            width, height);
        return;
    }

    if (width < MIN_DIMENSION || height < MIN_DIMENSION) {
        log(LogLevel::WARNING, "Window resolution %dx%d is below minimum %u. Clamping.",
            width, height, MIN_DIMENSION);
        width = std::max(width, (int)MIN_DIMENSION);
        height = std::max(height, (int)MIN_DIMENSION);
    }

    if (width > MAX_DIMENSION || height > MAX_DIMENSION) {
        log(LogLevel::WARNING, "Window resolution %dx%d exceeds maximum %u. Clamping.",
            width, height, MAX_DIMENSION);
        width = std::min(width, (int)MAX_DIMENSION);
        height = std::min(height, (int)MAX_DIMENSION);
    }

    m_windowResolution = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    switch(m_currentMode) {
        case ResolutionMode::NATIVE:
            m_renderResolution = m_windowResolution;
            m_upscaleRatio = 1.0f;
            break;

        case ResolutionMode::RES_240P: {
            if (height <= 0) {
                log(LogLevel::ERROR, "Invalid height for RES_240P calculation: %d", height);
                m_renderResolution = m_windowResolution;
                m_upscaleRatio = 1.0f;
                break;
            }
            float aspect = width / static_cast<float>(height);
            m_renderResolution.y = 240;
            uint32_t calculatedWidth = static_cast<uint32_t>(240 * aspect);
            m_renderResolution.x = std::clamp(calculatedWidth, 64u, 32768u);
            m_upscaleRatio = std::clamp(height / 240.0f, 0.5f, 136.5f);
            break;
        }

        case ResolutionMode::RES_480P: {
            if (height <= 0) {
                log(LogLevel::ERROR, "Invalid height for RES_480P calculation: %d", height);
                m_renderResolution = m_windowResolution;
                m_upscaleRatio = 1.0f;
                break;
            }
            float aspect = width / static_cast<float>(height);
            m_renderResolution.y = 480;
            uint32_t calculatedWidth = static_cast<uint32_t>(480 * aspect);
            m_renderResolution.x = std::clamp(calculatedWidth, 64u, 32768u);
            m_upscaleRatio = std::clamp(height / 480.0f, 0.25f, 68.27f);
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

float ResolutionManager::getPotencialUpscaleRatio(){
    if (m_windowResolution.x == 0 || m_windowResolution.y == 0) {
        log(LogLevel::ERROR, "getPotencialUpscaleRatio() called with invalid window resolution: %ux%u",
            m_windowResolution.x, m_windowResolution.y);
        m_renderResolution = m_windowResolution;
        m_upscaleRatio = 1.0f;
        return 1.0f;
    }

    int yscale = static_cast<int>(floor(m_windowResolution.y / 240.0f));
    int maxScale = std::max(1, yscale);

    return std::floor(maxScale);
}

void ResolutionManager::calculatePS1SharpResolution() {
    if (m_windowResolution.x == 0 || m_windowResolution.y == 0) {
        log(LogLevel::ERROR, "calculatePS1SharpResolution() called with invalid window resolution: %ux%u. Using native resolution.",
            m_windowResolution.x, m_windowResolution.y);
        m_renderResolution = m_windowResolution;
        m_upscaleRatio = 1.0f;
        return;
    }

    float maxScale = getPotencialUpscaleRatio();

    m_renderResolution = m_windowResolution / static_cast<unsigned int>(maxScale);
    m_upscaleRatio = maxScale;

    if (m_renderResolution.y < 240) {
        if (m_windowResolution.y <= 0) {
            log(LogLevel::ERROR, "Invalid window height for PS1_SHARP aspect calculation: %u", m_windowResolution.y);
            m_renderResolution = m_windowResolution;
            m_upscaleRatio = 1.0f;
            return;
        }

        float aspect = m_windowResolution.x / static_cast<float>(m_windowResolution.y);
        m_renderResolution.y = 240;
        uint32_t calculatedWidth = static_cast<uint32_t>(240 * aspect);
        m_renderResolution.x = std::clamp(calculatedWidth, 64u, 32768u);
        m_upscaleRatio = std::clamp(m_windowResolution.y / 240.0f, 0.5f, 136.5f);
    }
}

} // namespace vex
