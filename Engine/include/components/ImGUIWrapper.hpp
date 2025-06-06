#pragma once

#include <SDL3/SDL.h>
#include <functional>
#include <memory>
#include <vector>

#if DEBUG_BUILD
#include <imgui.h>
#endif

namespace vex {
    class ImGUIWrapper {
    public:
        virtual ~ImGUIWrapper() = default;

        virtual void init() = 0;
        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;
        virtual void processEvent(const SDL_Event* event) = 0;
#if DEBUG_BUILD
        virtual ImGuiIO& getIO() = 0;
#endif

        void addUIFunction(const std::function<void()>& func) {
            m_uiFunctions.push_back(func);
        }

        void executeUIFunctions() {
            for (auto& func : m_uiFunctions) {
                func();
            }
        }

    protected:
        std::vector<std::function<void()>> m_uiFunctions;
    };
}
