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
            /// @brief Constructor for Window class. Creates a new SDL_Window with the given title and resolution.
            Window(std::string title, int resx = 480, int resy = 640);
            ~Window();

            /// @brief Getter for SDL_Window pointer.
            /// @return SDL_Window*
            SDL_Window* GetSDLWindow();

            /// @brief Setter for fullscreen mode.
            /// @param bool enabled - True to enable fullscreen, false to disable.
            /// @param bool exclusive - True for exclusive fullscreen, false for borderless (Defaults to false)
            void setFullscreen(bool enabled, bool exclusive = false);

            /// @brief Getter for fullscreen mode.
            /// @return bool - True if fullscreen is enabled, false otherwise.
            bool isFullscreen();

            /// @brief Gets the refresh rate of the display the window is currently on.
            /// @return float - Refresh rate in Hz (e.g., 60.0, 144.0). Returns 60.0 on failure.
            float getRefreshRate();
    };
}
