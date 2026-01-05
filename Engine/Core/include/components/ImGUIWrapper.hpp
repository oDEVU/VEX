/**
 *  @file   ImGUIWrapper.hpp
 *  @brief  This file defines ImGUIWrapper class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <functional>
#include <memory>
#include <vector>

#if DEBUG
#include <imgui.h>
#include <ImGuizmo.h>
#endif

namespace vex {
    /// @brief This class provides an interface template for ImGui Implementation. Every backend should implement this class.
    class ImGUIWrapper {
    public:
        virtual ~ImGUIWrapper() = default;

        /// @brief Initialization called inside Engine class
        virtual void init() = 0;
        /// @brief Begining of UI rendering called in vex::Renderer::renderFrame
        virtual void beginFrame() = 0;
        /// @brief Ends UI rendering for frame
        virtual void endFrame() = 0;
        /// @brief Its used to process input needed by interaction with ImGui
        virtual void processEvent(const SDL_Event* event) = 0;
#if DEBUG
        virtual ImGuiIO& getIO() = 0;
#endif

        /// @brief Adds a function to be executed during UI rendering
        /// @details Example usage:
        /// @code
        /// #if DEBUG
        ///         m_imgui->addUIFunction([this]() {
        ///             ImGui::Begin("Window name");
        ///             ImGui::Text("Example test");
        ///             ImGui::Text("FPS: %i", (int)fps);
        ///             ImGui::End();
        ///         });
        /// #endif
        /// @endcode
        /// @details "#if DEBUG" is needed cause release builds dont compile with imgui support.
        void addUIFunction(const std::function<void()>& func) {
            m_uiFunctions.push_back(func);
        }

        /// @brief Called between beginFrame and endFrame, executes all imgui functions added by addUIFunction.
        void executeUIFunctions() {
            for (auto& func : m_uiFunctions) {
                func();
            }
        }

    protected:
        std::vector<std::function<void()>> m_uiFunctions;
    };
}
