#include "Window.hpp"

namespace vex {
    Window::Window(std::string title, int resx, int resy){
        log("Initializing SDL...");

        #ifdef __linux__
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
        #endif

        if (!SDL_Init(SDL_INIT_VIDEO)) {
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
            SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE
        );
        if (!window) {
            throw_error(SDL_GetError());
        }

        if (window == NULL) {
            log(LogLevel::ERROR, "Could not create window: %s", SDL_GetError());
            return;
        }
    }

    Window::~Window(){
        SDL_DestroyWindow(window);
        SDL_Quit();

        std::cout << "Window closed" << std::endl;
    }

    SDL_Window* Window::GetSDLWindow(){
        return window;
    }

    void Window::setFullscreen(bool enabled, bool exclusive) {
        if (m_cachedFullscreenState == enabled && m_cachedExclusiveMode == exclusive) {
            return;
        }

        Uint64 currentTime = SDL_GetPerformanceCounter();
        Uint64 timeSinceLastChangeMS = (currentTime - m_lastFullscreenChangeTime) / (SDL_GetPerformanceFrequency() / 1000);
        
        if (timeSinceLastChangeMS < FULLSCREEN_DEBOUNCE_MS) {
            log(LogLevel::WARNING, "Fullscreen toggle requested too rapidly (debounced). Last toggle was %lld ms ago, minimum interval is %llu ms.",
                timeSinceLastChangeMS, FULLSCREEN_DEBOUNCE_MS);
            return;
        }

        m_lastFullscreenChangeTime = currentTime;

        log("Attempting fullscreen state change: %s (mode: %s)",
            enabled ? "ENTER fullscreen" : "EXIT fullscreen",
            exclusive ? "exclusive" : "borderless windowed");

        if (enabled) {
            if (exclusive) {
                const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
                if (mode) {
                    SDL_SetWindowFullscreenMode(window, mode);
                    log("Set exclusive fullscreen mode: %ux%u @ %.1f Hz",
                        mode->w, mode->h, mode->refresh_rate);
                } else {
                    log(LogLevel::WARNING, "Could not query current display mode for exclusive fullscreen");
                }
            } else {
                SDL_SetWindowFullscreenMode(window, NULL);
                log("Set borderless windowed fullscreen mode");
            }

            if (!SDL_SetWindowFullscreen(window, true)) {
                log(LogLevel::ERROR, "Failed to enter fullscreen: %s", SDL_GetError());
                return;
            }
            log("Successfully entered fullscreen mode");
        } else {
            if (!SDL_SetWindowFullscreen(window, false)) {
                log(LogLevel::ERROR, "Failed to exit fullscreen: %s", SDL_GetError());
                return;
            }
            log("Successfully exited fullscreen mode");
        }

        m_cachedFullscreenState = enabled;
        m_cachedExclusiveMode = exclusive;
    }

    bool Window::isFullscreen() {
        return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
    }

    float Window::getRefreshRate() {
        SDL_DisplayID displayID = SDL_GetDisplayForWindow(window);
        if (displayID == 0) {
            log(LogLevel::ERROR,  "Could not get display for window: %s", SDL_GetError());
            return 60.0f;
        }

        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
        if (mode) {
            return mode->refresh_rate > 0.0f ? mode->refresh_rate : 60.0f;
        }

        return 60.0f;
    }
}