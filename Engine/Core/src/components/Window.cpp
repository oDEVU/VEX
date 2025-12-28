#include "Window.hpp"

namespace vex {
    Window::Window(std::string title, int resx, int resy){
        log("Initializing SDL...");

        #ifdef __linux__
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
        #endif

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            const char* error = SDL_GetError();
            log("SDL_Init failed: %s", error);
            throw_error(error);
        }

        const char* driver = SDL_GetCurrentVideoDriver();
        log("Active Video Driver: %s", driver ? driver : "Unknown");

        log("Creating window with resolution: %i X %i", resx, resy);

        log("Creating window...");
        window = SDL_CreateWindow(
            title.c_str(),
            resx,
            resy,
            SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE // currently vulkan hardcoded
        );
        if (!window) {
            throw_error(SDL_GetError());
        }

        if (window == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
            return;
        }
    }

    Window::~Window(){
        // CleanUp

        SDL_DestroyWindow(window);
        SDL_Quit();

        std::cout << "Window closed" << std::endl;
    }

    SDL_Window* Window::GetSDLWindow(){
        return window;
    }

    void Window::setFullscreen(bool enabled, bool exclusive) {
        if (enabled) {
            if (exclusive) {
                const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
                if (mode) {
                    SDL_SetWindowFullscreenMode(window, mode);
                }
            } else {
                SDL_SetWindowFullscreenMode(window, NULL);
            }

            if (!SDL_SetWindowFullscreen(window, true)) {
                log(LogLevel::ERROR, "Failed to enter fullscreen: %s", SDL_GetError());
            }
        } else {
            if (!SDL_SetWindowFullscreen(window, false)) {
                log(LogLevel::ERROR, "Failed to exit fullscreen: %s", SDL_GetError());
            }
        }
    }

    bool Window::isFullscreen() {
        return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
    }
}
