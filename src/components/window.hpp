#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

namespace vex {
    class Window{
        private:
            SDL_Window *window;                    // Declare a pointer
            bool done = false;
        public:
            Window(std::string title, int resx = 480, int resy = 640);
            ~Window();

            SDL_Window* GetSDLWindow();
    };
}

#endif
