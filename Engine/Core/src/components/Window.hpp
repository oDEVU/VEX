/**
 *  @file   Window.hpp
 *  @brief  This file defines Window class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

#include "components/errorUtils.hpp"

namespace vex {
    /// @brief This class esentially abstracts SDL_Window
    class Window{
        private:
            SDL_Window *window;
            bool done = false;
        public:
            /// @brief Constructor for Window class.
            /// @details Initializes SDL with Video and Gamepad subsystems.
            /// - On Linux, attempts to hint `wayland,x11` drivers.
            /// - Creates the window with flags: `SDL_WINDOW_VULKAN`, `SDL_WINDOW_HIGH_PIXEL_DENSITY`, and `SDL_WINDOW_RESIZABLE`.
            /// @throws std::runtime_error - If SDL initialization or Window creation fails.
            /// @param std::string title - Title of the window.
            /// @param int resx - Width of the window in pixels (default: 480).
            /// @param int resy - Height of the window in pixels (default: 640).
            Window(std::string title, int resx = 480, int resy = 640);
            ~Window();

            /// @brief Getter for SDL_Window pointer.
            /// @return SDL_Window*
            SDL_Window* GetSDLWindow();

            /// @brief Setter for fullscreen mode.
            /// @details
            /// - **Exclusive**: Queries the current display mode and sets `SDL_SetWindowFullscreenMode` before entering fullscreen.
            /// - **Borderless**: Sets the mode to NULL (desktop fullscreen) before entering fullscreen.
            /// @param bool enabled - True to enable fullscreen, false to disable.
            /// @param bool exclusive - True for exclusive fullscreen (changes video mode), false for borderless windowed
            void setFullscreen(bool enabled, bool exclusive = false);

            /// @brief Getter for fullscreen mode.
            /// @return bool - True if fullscreen is enabled, false otherwise.
            bool isFullscreen();

            /// @brief Gets the refresh rate of the display the window is currently on.
            /// @details Queries `SDL_GetDisplayForWindow` followed by `SDL_GetCurrentDisplayMode`.
            /// @return float - Refresh rate in Hz. Returns 60.0f as a fallback if the query fails.
            float getRefreshRate();
    };
}
