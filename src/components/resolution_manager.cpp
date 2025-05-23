// resolution.cpp
#include "resolution_manager.hpp"

namespace vex {

ResolutionManager::ResolutionManager(SDL_Window* window) : window(window) {
    update();
}

void ResolutionManager::setMode(ResolutionMode mode) {
    currentMode = mode;
    update();
}

void ResolutionManager::update() {
    // Get current window size
    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    windowResolution = {width, height};

    switch(currentMode) {
        case ResolutionMode::NATIVE:
            renderResolution = windowResolution;
            upscaleRatio = 1.0f;
            break;

        case ResolutionMode::RES_240P: {
            float aspect = width / static_cast<float>(height);
            renderResolution.y = 240;
            renderResolution.x = static_cast<uint32_t>(240 * aspect);
            upscaleRatio = height / 240.0f;
            break;
        }

        case ResolutionMode::RES_480P: {
            float aspect = width / static_cast<float>(height);
            renderResolution.y = 480;
            renderResolution.x = static_cast<uint32_t>(480 * aspect);
            upscaleRatio = height / 480.0f;
            break;
        }

        case ResolutionMode::PS1_SHARP:
            calculatePS1SharpResolution();
            break;
    }

    SDL_Log("Resolution mode updated: %dx%d (render) -> %dx%d (window), scale: %.2f",
           renderResolution.x, renderResolution.y,
           windowResolution.x, windowResolution.y,
           upscaleRatio);
}

void ResolutionManager::calculatePS1SharpResolution() {
    // Find the largest integer scale factor that gives us at least 240p
    int yscale = windowResolution.y / 240;
    int maxScale = std::max(1, yscale);

    // Clamp between 1x and 4x scaling
    maxScale = std::clamp(maxScale, 1, 4);

    renderResolution = windowResolution / static_cast<unsigned int>(maxScale);
    upscaleRatio = maxScale;

    // Ensure minimum size of 320x240 (PS1 common resolution)
    if (renderResolution.x < 320 || renderResolution.y < 240) {
        float aspect = windowResolution.x / static_cast<float>(windowResolution.y);
        renderResolution.y = 240;
        renderResolution.x = static_cast<uint32_t>(240 * aspect);
        upscaleRatio = windowResolution.y / 240.0f;
    }
}

} // namespace vex
