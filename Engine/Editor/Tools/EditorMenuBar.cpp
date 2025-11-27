#include "EditorMenuBar.hpp"
#include "../DialogWindow.hpp"
#include "components/GameInfo.hpp"
#include "components/errorUtils.hpp"
#include "nlohmann/detail/value_t.hpp"

void EditorMenuBar::DrawBar(){
    std::erase_if(m_Windows, [](const std::shared_ptr<BasicEditorWindow>& window) {
        return !window->isOpen;
    });

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) {
                vex::log("New Scene");
            }
            if (ImGui::MenuItem("Open Scene")) {
                OpenScene();
            }
            if (ImGui::MenuItem("Save Scene")) {}
            if (ImGui::MenuItem("Save All Scenes")) {}
            if (ImGui::MenuItem("Quit Project")) {
                // Implemet saving window if not saved
                // std::quick_exit(0);
                m_engine.quit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo")) {}
            if (ImGui::MenuItem("Redo")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Preferences")) {
            if (ImGui::MenuItem("Project Settings")) {}
            if (ImGui::MenuItem("Editor Settings")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void EditorMenuBar::OpenScene(){
    std::shared_ptr<BasicEditorWindow> openSceneWindow = std::make_shared<BasicEditorWindow>();
    openSceneWindow->Create = [openSceneWindow](vex::ImGUIWrapper& wrapper){
        wrapper.addUIFunction([&](){
            if (!openSceneWindow->isOpen) return;
            ImGui::Begin("Open Scene", &openSceneWindow->isOpen);
            ImGui::Text("[Move it to custom class]");
            ImGui::End();
        });
    };
    m_Windows.push_back(openSceneWindow);
    openSceneWindow->Create(m_ImGUIWrapper);
}
