#include "EditorMenuBar.hpp"
#include "AssetBrowser.hpp"

#include "../Editor.hpp"
#include "../DialogWindow.hpp"
#include "../editorProperties.hpp"
#include "../execute.hpp"

#include "components/GameInfo.hpp"
#include "components/errorUtils.hpp"
#include "components/Scene.hpp"
#include "components/SceneManager.hpp"
#include "imgui.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <ImReflect.hpp>

void EditorMenuBar::DrawBar(){
    std::erase_if(m_Windows, [](const std::shared_ptr<BasicEditorWindow>& window) {
        return !window->isOpen;
    });

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) {
                NewScene();
            }
            if (ImGui::MenuItem("Open Scene")) {
                OpenScene();
            }
            if (ImGui::MenuItem("Save Scene")) {
                m_editor.getSceneManager()->GetScene(m_editor.getSceneManager()->getLastSceneName())->Save(m_editor.getSceneManager()->getLastSceneName());
            }
            if (ImGui::MenuItem("Save Scene As")) {
                SaveSceneAs();
            }
            if (ImGui::MenuItem("Quit Project")) {
                // Implemet saving window if not saved
                // std::quick_exit(0);
                m_editor.quit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Build")) {
            if (ImGui::MenuItem("Build Debug")) {
                RunBuild(true, false);
            }
            if (ImGui::MenuItem("Build Release")) {
                RunBuild(false, false);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Preferences")) {
            if (ImGui::MenuItem("Editor Settings")) {
                OpenEditorSettings();
            }
            if (ImGui::MenuItem("Project Settings")) {}
            ImGui::EndMenu();
        }

        float runMenuWidth = ImGui::CalcTextSize("Run").x + 40.0f;
        float currentPos = ImGui::GetCursorPosX();
        float rightPos = ImGui::GetWindowWidth() - runMenuWidth;

        if (rightPos > currentPos) {
            ImGui::SetCursorPosX(rightPos);
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.00f, 0.23f, 0.01f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.47f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.03f, 0.03f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.85f, 0.85f, 1.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

        if (ImGui::Button("Run", ImVec2(runMenuWidth - 10, 0))) {
            ImGui::OpenPopup("RunPopup");
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        if (ImGui::BeginPopup("RunPopup")) {
            if (ImGui::MenuItem("Debug")) {
                RunBuild(true, true);
            }
            if (ImGui::MenuItem("Release")) {
                RunBuild(false, true);
            }
            ImGui::EndPopup();
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

void EditorMenuBar::SaveSceneAs() {
    std::shared_ptr<BasicEditorWindow> openSceneWindow = std::make_shared<BasicEditorWindow>();
    std::string startPath = m_editor.getProjectBinaryPath() + "/Assets";
    auto dialogBrowser = std::make_shared<vex::AssetBrowser>(startPath);
    auto showError = std::make_shared<bool>(false);

    auto saveFolder = std::make_shared<std::string>(startPath);
    auto fileName   = std::make_shared<std::string>("NewScene.json");

    auto* scene = m_editor.getSceneManager()->GetScene(m_editor.getSceneManager()->getLastSceneName());

    openSceneWindow->Create = [this, openSceneWindow, dialogBrowser, showError, scene, saveFolder, fileName](vex::ImGUIWrapper& wrapper) {
        wrapper.addUIFunction([=, this]() {
            if (!openSceneWindow->isOpen) return;
            ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Save Scene As", &openSceneWindow->isOpen)) {

                std::string selectedFile = dialogBrowser->Draw(m_editor.GetEditorIcons());

                if (!selectedFile.empty()) {
                    std::filesystem::path p(selectedFile);

                    if (p.has_extension()) {
                        *saveFolder = p.parent_path().string();
                        *fileName   = p.filename().string();
                    } else {
                        *saveFolder = selectedFile;
                    }
                }
                else if (dialogBrowser->getCurrentPath() != *saveFolder) {
                    *saveFolder = dialogBrowser->getCurrentPath();
                }

                ImGui::Separator();
                ImGui::Text("Save Options");

                ImReflect::Input("Filename", *fileName);

                if (ImGui::Button("Save", ImVec2(100, 0))) {
                    if (saveFolder->empty() || fileName->empty()) {
                        vex::log("Error: Folder or Filename cannot be empty.");
                    }
                    else {
                        std::filesystem::path finalPath(*saveFolder);
                        finalPath /= *fileName;

                        if (!finalPath.has_extension()) {
                            finalPath += ".json";
                        }

                        scene->Save(finalPath.string());
                        vex::log("Saved scene to: %s", finalPath.string().c_str());
                        openSceneWindow->isOpen = false;
                    }
                }
            }
            ImGui::End();
        });
    };

    m_Windows.push_back(openSceneWindow);
    openSceneWindow->Create(m_ImGUIWrapper);
}

std::string EditorMenuBar::GetProjectName() {
    std::filesystem::path projectFile = std::filesystem::path(m_editor.getProjectBinaryPath()) / "VexProject.json";

    std::ifstream file(projectFile);
    if (file.is_open()) {
        try {
            nlohmann::json j;
            file >> j;
            if (j.contains("project_name")) {
                return j["project_name"].get<std::string>();
            }
        } catch (...) {
            vex::log("Error: Failed to parse VexProject.json");
        }
    }
    return "";
}

void EditorMenuBar::RunBuild(bool isDebug, bool runAfter) {
    struct BuildState {
        std::string logs = "";
        bool isFinished = false;
        std::mutex logMutex;
        bool autoScroll = true;
    };
    auto state = std::make_shared<BuildState>();

    std::string projectPath = m_editor.getProjectBinaryPath();
    std::string configFlag = isDebug ? " -d" : " -r";
    std::string command;

    #ifdef _WIN32
        command = "..\\..\\BuildTools\\build\\ProjectBuilder.exe \"" + projectPath + "\"" + configFlag;
    #else
        command = "../../BuildTools/build/ProjectBuilder \"" + projectPath + "\"" + configFlag;
    #endif

    vex::log("Starting Build: %s", command.c_str());

    std::shared_ptr<BasicEditorWindow> buildWindow = std::make_shared<BasicEditorWindow>();

    buildWindow->Create = [this, buildWindow, state, isDebug](vex::ImGUIWrapper& wrapper) {
        wrapper.addUIFunction([=]() {
            if (!buildWindow->isOpen) return;

            ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

            std::string title = isDebug ? "Building Debug..." : "Building Release...";
            if (state->isFinished) title += " (Finished)";

            if (ImGui::Begin(title.c_str(), &buildWindow->isOpen)) {

                if(ImGui::Button("Clear")) {
                    std::lock_guard<std::mutex> lock(state->logMutex);
                    state->logs.clear();
                }
                ImGui::SameLine();
                ImGui::Checkbox("Auto-scroll", &state->autoScroll);

                ImGui::Separator();

                ImGui::BeginChild("BuildLogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

                {
                    std::lock_guard<std::mutex> lock(state->logMutex);
                    ImGui::TextUnformatted(state->logs.c_str());
                }

                if (state->autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }

                ImGui::EndChild();
            }
            ImGui::End();
        });
    };

    m_Windows.push_back(buildWindow);
    buildWindow->Create(m_ImGUIWrapper);

    std::thread([command, state, this, isDebug, runAfter]() {
        executeCommandRealTime(command,
            [state](const std::string& line) {
                std::lock_guard<std::mutex> lock(state->logMutex);
                state->logs += line;
            }
        );

        if (runAfter) {
            std::string projectName = GetProjectName();
            if (projectName.empty()) {
                std::lock_guard<std::mutex> lock(state->logMutex);
                state->logs += "\n[Error] Could not retrieve Project Name from VexProject.json. Cannot run.\n";
            } else {
                std::filesystem::path runPath(m_editor.getProjectBinaryPath());
                runPath = runPath / "Build" / (isDebug ? "Debug" : "Release");
                runPath = runPath / projectName;

                #ifdef _WIN32
                    runPath += ".exe";
                #endif

                if (std::filesystem::exists(runPath)) {
                    {
                        std::lock_guard<std::mutex> lock(state->logMutex);
                        state->logs += "\n=== Build Complete. Launching: " + runPath.string() + " ===\n";
                    }

                    std::string runCmd = "\"" + runPath.string() + "\"";

                    executeCommandRealTime(runCmd,
                        [state](const std::string& line) {
                            std::lock_guard<std::mutex> lock(state->logMutex);
                            state->logs += line;
                        }
                    );
                } else {
                    std::lock_guard<std::mutex> lock(state->logMutex);
                    state->logs += "\n[Error] Executable not found at: " + runPath.string() + "\n";
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(state->logMutex);
            state->logs += "\n=== Build Finished ===\n";
            state->isFinished = true;
        }
    }).detach();
}

void EditorMenuBar::NewScene() {
    std::shared_ptr<BasicEditorWindow> newSceneWindow = std::make_shared<BasicEditorWindow>();
    std::string startPath = m_editor.getProjectBinaryPath() + "/Assets";
    auto dialogBrowser = std::make_shared<vex::AssetBrowser>(startPath);

   auto saveFolder = std::make_shared<std::string>(startPath);
    auto fileName   = std::make_shared<std::string>("NewScene.json");

    newSceneWindow->Create = [this, newSceneWindow, dialogBrowser, saveFolder, fileName](vex::ImGUIWrapper& wrapper) {
        wrapper.addUIFunction([=, this]() {
            if (!newSceneWindow->isOpen) return;
            ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Create New Scene", &newSceneWindow->isOpen)) {

                std::string selectedFile = dialogBrowser->Draw(m_editor.GetEditorIcons());

                if (!selectedFile.empty()) {
                    std::filesystem::path p(selectedFile);
                    if (p.has_extension()) {
                        *saveFolder = p.parent_path().string();
                        *fileName   = p.filename().string();
                    } else {
                        *saveFolder = selectedFile;
                    }
                }
                else if (dialogBrowser->getCurrentPath() != *saveFolder) {
                    *saveFolder = dialogBrowser->getCurrentPath();
                }

                ImGui::Separator();
                ImGui::Text("New Scene Options");

                ImReflect::Input("Filename", *fileName);

                if (ImGui::Button("Create & Open", ImVec2(120, 0))) {
                    if (saveFolder->empty() || fileName->empty()) {
                        vex::log("Error: Folder or Filename cannot be empty.");
                    }
                    else {
                        std::filesystem::path destPath(*saveFolder);
                        destPath /= *fileName;

                        if (!destPath.has_extension()) {
                            destPath += ".json";
                        }

                        std::filesystem::path sourcePath = std::filesystem::path("../Assets/default/default.json");

                        try {
                            if (std::filesystem::exists(sourcePath)) {
                                std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing);
                                vex::log("New scene created: %s", destPath.string().c_str());

                                m_editor.requestSceneReload(destPath.string());
                                newSceneWindow->isOpen = false;
                            }
                            else {
                                vex::log("Error: Could not find default scene template at: %s", sourcePath.string().c_str());
                            }
                        }
                        catch (const std::filesystem::filesystem_error& e) {
                            vex::log("Filesystem Error: %s", e.what());
                        }
                    }
                }
            }
            ImGui::End();
        });
    };

    m_Windows.push_back(newSceneWindow);
    newSceneWindow->Create(m_ImGUIWrapper);
}
