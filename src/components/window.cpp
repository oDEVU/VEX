#include "window.hpp"

namespace vex {
    Window::Window(std::string title, int resx, int resy){
        SDL_Log("Initializing SDL...");
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            const char* error = SDL_GetError();
            SDL_Log("SDL_Init failed: %s", error);
            throw std::runtime_error(error);
        }

        std::cout << "resx: " << resx << std::endl;
        std::cout << "resy: " << resy << std::endl;

        // Create an application window with the following settings:
        SDL_Log("Creating window...");
        window = SDL_CreateWindow(
            title.c_str(),                  // window title
            resx,                               // width, in pixels
            resy,                               // height, in pixels
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE // currently vulkan hardcoded
        );
        if (!window) {
            throw std::runtime_error(SDL_GetError());
        }

        // Check that the window was successfully created
        if (window == NULL) {
            // In the case that the window could not be made...
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
            return;
        }
    }

    Window::~Window(){
        // Close and destroy the window
        SDL_DestroyWindow(window);

        // Clean up
        SDL_Quit();

        std::cout << "Window closed" << std::endl;
    }

    SDL_Window* Window::GetSDLWindow(){
        return window;
    }
}
