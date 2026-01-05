/**
 *  @file   ResolutionManager.hpp
 *  @brief  This file defines ResolutionManager class and ResolutionMode struct.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

namespace vex {

class Window;

/// @brief Enumeration representing different resolution modes.
enum class ResolutionMode {
    /// @brief Use window resolution.
    NATIVE,
    /// @brief 240p (426x240 for 16:9).
    RES_240P,
    /// @brief 480p (854x480 for 16:9).
    RES_480P,
    /// @brief Integer-scaled resolution closest to PS1.
    PS1_SHARP
};

/// @brief Class responsible for managing resolution settings, calculating render resolution based on selected mode.
class ResolutionManager {
public:
    /// @brief Constructor.
    /// @param SDL_Window* p_window - Pointer to the SDL window.
    ResolutionManager(SDL_Window *p_window);

    /// @brief Function to set the resolution mode. It is called by engine method.
    /// @param ResolutionMode mode - The resolution mode to set.
    void setMode(ResolutionMode mode);

    /// @brief Function to get the current resolution mode.
    ResolutionMode getCurrentMode() const { return m_currentMode; }

    /// @brief Function to get the current render resolution.
    /// @return glm::uvec2 - The current render resolution.
    glm::uvec2 getRenderResolution() const { return m_renderResolution; }

    /// @brief Function to get the current window resolution.
    /// @return glm::uvec2 - The current window resolution.
    glm::uvec2 getWindowResolution() const { return m_windowResolution; }

    /// @brief Function to get the current upscale ratio. Read window resolution divided by render resolution.
    /// @return float - The current upscale ratio.
    float getUpscaleRatio() const { return m_upscaleRatio; }

    /// @brief Updates the internal resolution state based on the current window size and mode.
    /// @details Queries the actual window size from SDL (`SDL_GetWindowSizeInPixels`). Depending on `m_currentMode`:
    /// - **NATIVE**: Matches render resolution to window size (1:1).
    /// - **RES_240P/480P**: Fixes the vertical resolution to 240 or 480 and calculates width based on aspect ratio.
    /// - **PS1_SHARP**: Calls `calculatePS1SharpResolution()` to find an integer scale factor.
    void update();

private:
    /// @brief Internal helper to calculate integer-scaled resolutions for the PS1_SHARP mode.
    /// @details Determines the maximum integer scale factor (clamped between 1 and 4) that fits within the current window height.
    /// Ensures the render height does not drop below 240p.
    void calculatePS1SharpResolution();

    SDL_Window *m_p_window;
    ResolutionMode m_currentMode = ResolutionMode::PS1_SHARP;
    glm::uvec2 m_windowResolution;
    glm::uvec2 m_renderResolution;
    float m_upscaleRatio = 1.0f;
};

} // namespace vex
