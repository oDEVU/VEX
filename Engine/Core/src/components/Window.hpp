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
    };
}
