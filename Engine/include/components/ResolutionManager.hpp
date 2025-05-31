#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <bits/stdc++.h>
#include <SDL3/SDL.h>

namespace vex {

class Window;

enum class ResolutionMode {
    NATIVE,     // Use window resolution
    RES_240P,   // 240p (426x240 for 16:9)
    RES_480P,   // 480p (854x480 for 16:9)
    PS1_SHARP   // Integer-scaled resolution closest to PS1
};

class ResolutionManager {
public:
    ResolutionManager(SDL_Window* window);

    void setMode(ResolutionMode mode);
    ResolutionMode getCurrentMode() const { return currentMode; }

    glm::uvec2 getRenderResolution() const { return renderResolution; }
    glm::uvec2 getWindowResolution() const { return windowResolution; }
    float getUpscaleRatio() const { return upscaleRatio; }

    void update();

private:
    void calculatePS1SharpResolution();

    SDL_Window* window;
    ResolutionMode currentMode = ResolutionMode::PS1_SHARP;
    glm::uvec2 windowResolution;
    glm::uvec2 renderResolution;
    float upscaleRatio = 1.0f;
};

} // namespace vex
