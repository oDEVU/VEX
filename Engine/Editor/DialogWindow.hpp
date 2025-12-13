/**
 * @file   DialogWindow.hpp
 * @brief  A specialized Engine class for creating and managing modal dialog windows.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "Engine.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include <memory>

namespace vex {

    struct SceneRenderData;

    /// @brief A specialized Engine class for creating and managing modal dialog windows.
    class DialogWindow : public Engine {
    public:
        /// @brief Constructs a DialogWindow.
        /// @param const char* title - The title of the dialog window.
        /// @param GameInfo gInfo - The GameInfo object.
        DialogWindow(const char* title, GameInfo gInfo);
        ~DialogWindow();

        /// @brief Overrides the Engine's render function to draw the dialog content.
        void render() override;

        /// @brief Cleans up and shuts down the core engine components used by the dialog.
        void close(){
            m_imgui.reset();
            //m_interface.reset();
            m_inputSystem.reset();
            m_window.reset();
            SDL_Quit();
        }

        /// @brief Sets the internal running flag to false, effectivelly stopping the dialog's run loop.
        void stop(){
            m_running = false;
        }

        /// @brief Checks if the dialog window is currently running.
        /// @return bool - True if running, false otherwise.
        bool isRunning() const {
            return m_running;
        }

        /// @brief Sets the content string to be displayed in the dialog.
        /// @param const std::string& content - The message content.
        void setDialogContent(const std::string& content) {
            m_dialogContent = content;
        }

    private:
        /// @brief Internal helper to draw the ImGUI layout for the dialog window.
        /// @param const SceneRenderData& data - Data required for scene rendering (unused in basic dialog).
        /// @param glm::uvec2& outNewResolution - Output parameter for new resolution (unused in basic dialog).
        void drawDialogWindowLayout(const SceneRenderData& data, glm::uvec2& outNewResolution);
        std::unique_ptr<CameraObject> dummyCamera;
        std::string m_dialogContent = "";
    };

}
