#include "Window.hpp"

namespace vex {
    Window::Window(std::string title, int resx, int resy){
        SDL_Log("Initializing SDL...");

        // WHY THE FUCK WAYLAND IS SO BLURRY NO MATTER WHAT I DO?!?!?
        #ifdef __linux__
            SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "1");
            SDL_SetHint("SDL_HINT_RENDER_SCALE_QUALITY", "nearest");
            SDL_SetHint("SDL_VIDEO_WAYLAND_SCALE_TO_DISPLAY", "0");
            SDL_SetHint("SDL_RENDER_DRIVER", "vulkan");
            SDL_SetHint("SDL_VIDEO_FORCE_PIXEL_SIZE", "1");
            SDL_SetHint("SDL_VIDEO_WAYLAND_SCALE_TO_DISPLAY", "0");
            SDL_SetHint("SDL_VIDEO_WAYLAND_MODE_EMULATION", "1");
            SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland,x11", SDL_HINT_OVERRIDE);
            setenv("SDL_VIDEODRIVER", "wayland,x11", 1);
        #endif

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            const char* error = SDL_GetError();
            SDL_Log("SDL_Init failed: %s", error);
            throw std::runtime_error(error);
        }

        std::cout << "resx: " << resx << std::endl;
        std::cout << "resy: " << resy << std::endl;

        SDL_Log("Creating window...");
        window = SDL_CreateWindow(
            title.c_str(),
            resx,
            resy,
            SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE // currently vulkan hardcoded
        );
        if (!window) {
            throw std::runtime_error(SDL_GetError());
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
}
