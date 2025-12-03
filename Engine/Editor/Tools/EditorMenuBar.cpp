#include "EditorMenuBar.hpp"
#include "AssetBrowser.hpp"

#include "../Editor.hpp"
#include "../DialogWindow.hpp"
#include "../editorProperties.hpp"

#include "components/GameInfo.hpp"
#include "components/errorUtils.hpp"

#include <nlohmann/json.hpp>
#include <ImReflect.hpp>

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
            if (ImGui::MenuItem("Save Scene As")) {}
            if (ImGui::MenuItem("Save All Scenes")) {}
            if (ImGui::MenuItem("Quit Project")) {
                // Implemet saving window if not saved
                // std::quick_exit(0);
                m_editor.quit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Build")) {
            if (ImGui::MenuItem("Build Debug")) {}
            if (ImGui::MenuItem("Build Release")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Preferences")) {
            if (ImGui::MenuItem("Editor Settings")) {
                OpenEditorSettings();
            }
            if (ImGui::MenuItem("Project Settings")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void EditorMenuBar::OpenEditorSettings(){
    std::shared_ptr<BasicEditorWindow> openSceneWindow = std::make_shared<BasicEditorWindow>();

    EditorProperties* editorProperties = m_editor.getEditorProperties();

        openSceneWindow->Create = [this, openSceneWindow, editorProperties](vex::ImGUIWrapper& wrapper){
            wrapper.addUIFunction([=, this](){
                if (!openSceneWindow->isOpen) return;
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

                if (ImGui::Begin("Editor Settings", &openSceneWindow->isOpen)) {
                    ImReflect::Input("##data", editorProperties);
                }
                ImGui::End();
            });
        };

        m_Windows.push_back(openSceneWindow);
        openSceneWindow->Create(m_ImGUIWrapper);
}

void EditorMenuBar::OpenScene(){
    std::shared_ptr<BasicEditorWindow> openSceneWindow = std::make_shared<BasicEditorWindow>();
    std::string startPath = m_editor.getProjectBinaryPath() + "/Assets";
    auto dialogBrowser = std::make_shared<vex::AssetBrowser>(startPath);
    auto showError = std::make_shared<bool>(false);

    openSceneWindow->Create = [this, openSceneWindow, dialogBrowser, showError](vex::ImGUIWrapper& wrapper){
        wrapper.addUIFunction([=, this](){
            if (!openSceneWindow->isOpen) return;
            ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Open Scene File", &openSceneWindow->isOpen)) {
                std::string selectedFile = dialogBrowser->Draw(m_editor.GetEditorIcons());
                if (!selectedFile.empty()) {

                    if (dialogBrowser->GetExtension(selectedFile) == ".json") {

                        int type = dialogBrowser->GetJSONAssetType(selectedFile);
                        if (type == 1) {
                            vex::log("Menu Bar requesting load: %s", selectedFile.c_str());
                            m_editor.requestSceneReload(selectedFile);
                            openSceneWindow->isOpen = false;
                        } else {
                            *showError = true;
                        }
                    } else {
                        *showError = true;
                    }
                }

                if (*showError) {
                    ImGui::OpenPopup("Invalid Scene File");
                    *showError = false;
                }

                if (ImGui::BeginPopupModal("Invalid Scene File", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("The selected file is not a valid VEX Scene file.");
                    ImGui::Text("Please select a .json file with scene data.");

                    ImGui::Separator();

                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::End();
        });
    };

    m_Windows.push_back(openSceneWindow);
    openSceneWindow->Create(m_ImGUIWrapper);
}
