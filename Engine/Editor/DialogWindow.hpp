#pragma once
#include "Engine.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include <memory>

namespace vex {

    struct SceneRenderData;

    class DialogWindow : public Engine {
    public:
        DialogWindow(const char* title, GameInfo gInfo);
        ~DialogWindow();

        void render() override;

        void close(){
            m_imgui.reset();
            //m_interface.reset();
            m_inputSystem.reset();
            m_window.reset();
            SDL_Quit();
        }

        void stop(){
            m_running = false;
        }

        bool isRunning() const {
            return m_running;
        }

        void setDialogContent(const std::string& content) {
            m_dialogContent = content;
        }

    private:
        void drawDialogWindowLayout(const SceneRenderData& data, glm::uvec2& outNewResolution);
        std::unique_ptr<CameraObject> dummyCamera;
        std::string m_dialogContent = "";
    };

}
