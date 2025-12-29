#include "EditorMenuBar.hpp"
#include "AssetBrowser.hpp"

#include "../Editor.hpp"
#include "../DialogWindow.hpp"
#include "../editorProperties.hpp"
#include "../projectProperties.hpp"
#include "../execute.hpp"

#include "components/GameInfo.hpp"
#include "components/errorUtils.hpp"
#include "components/pathUtils.hpp"
#include "components/Scene.hpp"
#include "components/SceneManager.hpp"
#include "imgui.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <ImReflect.hpp>

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

void EditorMenuBar::DrawBar(){
    std::erase_if(m_Windows, [](const std::shared_ptr<BasicEditorWindow>& window) {
        return !window->isOpen;
    });

    static enum { ACTION_NONE, ACTION_QUIT, ACTION_TO_SELECTOR, ACTION_RUN_DEBUG, ACTION_RUN_RELEASE } pendingQuitAction = ACTION_NONE;
    bool openSavePopup = false;

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) {
                NewScene();
            }
            if (ImGui::MenuItem("Open Scene")) {
                OpenScene();
            }
            if (ImGui::MenuItem("Save Scene")) {
                std::string sceneName = vex::GetAssetPath(m_editor.getSceneManager()->getLastSceneName());
                m_editor.getSceneManager()->GetScene(sceneName)->Save(sceneName);
            }
            if (ImGui::MenuItem("Save Scene As")) {
                SaveSceneAs();
            }
            if (ImGui::MenuItem("Quit to Project Selector")) {
                pendingQuitAction = ACTION_TO_SELECTOR;
                openSavePopup = true;
            }
            if (ImGui::MenuItem("Quit Project")) {
                pendingQuitAction = ACTION_QUIT;
                openSavePopup = true;
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
            if (ImGui::MenuItem("Build Distribution (Shipping)")) {
                BuildDist();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Preferences")) {
            if (ImGui::MenuItem("Editor Settings")) {
                OpenEditorSettings();
            }
            if (ImGui::MenuItem("Project Settings")) {
                OpenProjectSettings();
            }
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
                pendingQuitAction = ACTION_RUN_DEBUG;
                openSavePopup = true;
            }
            if (ImGui::MenuItem("Release")) {
                pendingQuitAction = ACTION_RUN_RELEASE;
                openSavePopup = true;
            }
            ImGui::EndPopup();
        }
        ImGui::EndMenuBar();
    }

    if (openSavePopup) {
        ImGui::OpenPopup("Save Changes?");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Save Changes?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Do you want to save changes to the current scene before quitting?");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            std::string sceneName = vex::GetAssetPath(m_editor.getSceneManager()->getLastSceneName());
            m_editor.getSceneManager()->GetScene(sceneName)->Save(sceneName);
            ImGui::CloseCurrentPopup();

            if (pendingQuitAction == ACTION_TO_SELECTOR) {
                OpenProjectSelector();
                m_editor.quit();
            } else if (pendingQuitAction == ACTION_QUIT) {
                m_editor.quit();
            }else if (pendingQuitAction == ACTION_RUN_DEBUG) {
                RunBuild(true, true);
            }else if (pendingQuitAction == ACTION_RUN_RELEASE) {
                RunBuild(false, true);
            }
            pendingQuitAction = ACTION_NONE;
        }

        ImGui::SameLine();

        if (ImGui::Button("Don't Save", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();

            if (pendingQuitAction == ACTION_TO_SELECTOR) {
                OpenProjectSelector();
                m_editor.quit();
            } else if (pendingQuitAction == ACTION_QUIT) {
                m_editor.quit();
            }else if (pendingQuitAction == ACTION_RUN_DEBUG) {
                RunBuild(true, true);
            }else if (pendingQuitAction == ACTION_RUN_RELEASE) {
                RunBuild(false, true);
            }
            pendingQuitAction = ACTION_NONE;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            pendingQuitAction = ACTION_NONE;
        }

        ImGui::EndPopup();
    }
}

void EditorMenuBar::OpenProjectSelector() {
    std::filesystem::path binDir = vex::GetExecutableDir();
    std::filesystem::path selectorPath = binDir / "VexProjectSelector";

    #ifdef _WIN32
        selectorPath += ".exe";
    #endif

    if (!std::filesystem::exists(selectorPath)) {
        vex::log("Error: Could not find ProjectSelector at %s", selectorPath.string().c_str());
        return;
    }

    std::string pathStr = selectorPath.string();

    #ifdef _WIN32
        ShellExecuteA(NULL, "open", pathStr.c_str(), NULL, NULL, SW_SHOW);
    #else
        pid_t pid = fork();

        if (pid == 0) {
            if (setsid() < 0) {
                exit(EXIT_FAILURE);
            }
            execl(pathStr.c_str(), pathStr.c_str(), (char*)NULL);
            exit(EXIT_FAILURE);
        }
    #endif
}

void EditorMenuBar::OpenEditorSettings(){
    std::shared_ptr<BasicEditorWindow> openSceneWindow = std::make_shared<BasicEditorWindow>();
    std::weak_ptr<BasicEditorWindow> weakWindow = openSceneWindow;

    EditorProperties* editorProperties = m_editor.getEditorProperties();

        openSceneWindow->Create = [this, weakWindow, editorProperties](vex::ImGUIWrapper& wrapper){
            wrapper.addUIFunction([=, this](){
                auto window = weakWindow.lock(); if (!window || !window->isOpen) return;
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

                if (ImGui::Begin("Editor Settings", &window->isOpen)) {
                    ImReflect::Input("##data", editorProperties);
                }
                ImGui::End();
            });
        };

        m_Windows.push_back(openSceneWindow);
        openSceneWindow->Create(m_ImGUIWrapper);
}

void EditorMenuBar::OpenProjectSettings(){
    std::shared_ptr<BasicEditorWindow> openSceneWindow = std::make_shared<BasicEditorWindow>();
    std::weak_ptr<BasicEditorWindow> weakWindow = openSceneWindow;

    ProjectProperties* projectProperties = m_editor.getProjectProperties();

        openSceneWindow->Create = [this, weakWindow, projectProperties](vex::ImGUIWrapper& wrapper){
            wrapper.addUIFunction([=, this](){
                auto window = weakWindow.lock(); if (!window || !window->isOpen) return;
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

                if (ImGui::Begin("Project Settings", &window->isOpen)) {
                    ImReflect::Input("##data", projectProperties);
                }
                ImGui::End();
            });
        };

        m_Windows.push_back(openSceneWindow);
        openSceneWindow->Create(m_ImGUIWrapper);
}

void EditorMenuBar::OpenScene(){
    std::shared_ptr<BasicEditorWindow> openSceneWindow = std::make_shared<BasicEditorWindow>();
    std::weak_ptr<BasicEditorWindow> weakWindow = openSceneWindow;
    std::string startPath = m_editor.getProjectBinaryPath() + "/Assets";
    auto dialogBrowser = std::make_shared<vex::AssetBrowser>(startPath);
    auto showError = std::make_shared<bool>(false);

    openSceneWindow->Create = [this, weakWindow, dialogBrowser, showError](vex::ImGUIWrapper& wrapper){
        wrapper.addUIFunction([=, this](){
            auto window = weakWindow.lock(); if (!window || !window->isOpen) return;
            ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Open Scene File", &window->isOpen)) {
                std::string selectedFile = dialogBrowser->Draw(m_editor.GetEditorIcons());
                if (!selectedFile.empty() && !dialogBrowser->GetExtension(selectedFile).empty()) {

                    if (dialogBrowser->GetExtension(selectedFile) == ".json") {

                        int type = dialogBrowser->GetJSONAssetType(selectedFile);
                        if (type == 1) {
                            vex::log("Menu Bar requesting load: %s", selectedFile.c_str());
                            m_editor.requestSceneReload(selectedFile);
                            window->isOpen = false;
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
    std::weak_ptr<BasicEditorWindow> weakWindow = openSceneWindow;
    std::string startPath = m_editor.getProjectBinaryPath() + "/Assets";
    auto dialogBrowser = std::make_shared<vex::AssetBrowser>(startPath);
    auto showError = std::make_shared<bool>(false);

    auto saveFolder = std::make_shared<std::string>(startPath);
    auto fileName   = std::make_shared<std::string>("NewScene.json");

    auto* scene = m_editor.getSceneManager()->GetScene(m_editor.getSceneManager()->getLastSceneName());

    openSceneWindow->Create = [this, weakWindow, dialogBrowser, showError, scene, saveFolder, fileName](vex::ImGUIWrapper& wrapper) {
        wrapper.addUIFunction([=, this]() {
            auto window = weakWindow.lock(); if (!window || !window->isOpen) return;
            ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Save Scene As", &window->isOpen)) {

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
                        window->isOpen = false;
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
    std::weak_ptr<BasicEditorWindow> weakWindow = buildWindow;

    buildWindow->Create = [this, weakWindow, state, isDebug](vex::ImGUIWrapper& wrapper) {
        wrapper.addUIFunction([=]() {
            auto window = weakWindow.lock(); if (!window || !window->isOpen) return;

            ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

            std::string title = isDebug ? "Building Debug..." : "Building Release...";
            if (state->isFinished) title += " (Finished)";

            if (ImGui::Begin(title.c_str(), &window->isOpen)) {

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

void EditorMenuBar::BuildDist() {
    struct BuildState {
        std::string logs = "";
        bool isFinished = false;
        std::mutex logMutex;
        bool autoScroll = true;
    };
    auto state = std::make_shared<BuildState>();

    std::string projectPath;
    std::string enginePath;

    try {
        projectPath = std::filesystem::canonical(m_editor.getProjectBinaryPath()).string();
        enginePath = std::filesystem::canonical(vex::GetExecutableDir() / ".." / "..").string();
    } catch (const std::exception& e) {
        vex::log("Error resolving paths for Dist Build: %s", e.what());
        return;
    }

    std::string command;

    #ifdef _WIN32
        command = "..\\..\\BuildTools\\build\\ProjectBuilder.exe \"" + projectPath + "\" -dist";
        vex::log("Starting Windows Distribution Build: %s", command.c_str());

    #else
        std::string image = "vex-builder:latest";
        std::string containerEnginePath = "/VexEngine";
        std::string containerProjectPath = "/VexProject";

        std::string shellCmd =
                    "export VEX_CXX_COMPILER=clang++-17 && "
                    "echo '>> Compiling ProjectBuilder (Container)...' && "
                    "cd " + containerEnginePath + "/BuildTools && "
                    "chmod +x rebuild-buildtools-linux.sh && "
                    "./rebuild-buildtools-linux.sh build_dist && "
                    "echo '>> Starting Project Distribution Build...' && "
                    "./build_dist/ProjectBuilder " + containerProjectPath + " -dist";

        std::stringstream cmdBuilder;

        bool isPodman = (system("which podman > /dev/null 2>&1") == 0);
        if (isPodman) cmdBuilder << "podman";
        else cmdBuilder << "docker";

        std::string mountOpts = ":rw,z";

        cmdBuilder << " run --rm ";

        if (isPodman) {
            cmdBuilder << "--userns=keep-id ";
        } else {
            cmdBuilder << "--user $(id -u):$(id -g) ";
        }

        cmdBuilder << "-v \"" << enginePath << ":" << containerEnginePath << mountOpts << "\" "
            << "-v \"" << projectPath << ":" << containerProjectPath << mountOpts << "\" "
            << "-w " << containerEnginePath << " "
            << image << " "
            << "/bin/bash -c \"" << shellCmd << "\"";

        command = cmdBuilder.str() + " 2>&1";

        vex::log("Starting Linux (Sniper) Distribution Build...");
    #endif

    std::shared_ptr<BasicEditorWindow> buildWindow = std::make_shared<BasicEditorWindow>();
    std::weak_ptr<BasicEditorWindow> weakWindow = buildWindow;

    buildWindow->Create = [this, weakWindow, state](vex::ImGUIWrapper& wrapper) {
        wrapper.addUIFunction([=]() {
            auto window = weakWindow.lock(); if (!window || !window->isOpen) return;
            ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Distribution Build (Shipping)", &window->isOpen)) {
                if(ImGui::Button("Clear")) {
                    std::lock_guard<std::mutex> lock(state->logMutex);
                    state->logs.clear();
                }
                ImGui::SameLine();
                ImGui::Checkbox("Auto-scroll", &state->autoScroll);
                ImGui::Separator();

                #ifndef _WIN32
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[Linux] Building in Special Container.");
                #endif

                if (state->isFinished) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Finished");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Status: Working...");
                }

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

    std::thread([command, state]() {
        try {
            executeCommandRealTime(command,
                [state](const std::string& line) {
                    {
                        std::lock_guard<std::mutex> lock(state->logMutex);
                        state->logs += line;
                    }

                    if (line.length() > 1) {
                        vex::log("%s", line.c_str());
                    }
                }
            );
        } catch (const std::exception& e) {
             std::lock_guard<std::mutex> lock(state->logMutex);
             state->logs += "\n[CRITICAL ERROR] Execution failed: ";
             state->logs += e.what();
        }

        {
            std::lock_guard<std::mutex> lock(state->logMutex);
            state->logs += "\n=== Distribution Build Finished ===\n";
            state->isFinished = true;
        }
    }).detach();
}

void EditorMenuBar::NewScene() {
    std::shared_ptr<BasicEditorWindow> newSceneWindow = std::make_shared<BasicEditorWindow>();
    std::weak_ptr<BasicEditorWindow> weakWindow = newSceneWindow;
    std::string startPath = m_editor.getProjectBinaryPath() + "/Assets";
    auto dialogBrowser = std::make_shared<vex::AssetBrowser>(startPath);

   auto saveFolder = std::make_shared<std::string>(startPath);
    auto fileName   = std::make_shared<std::string>("NewScene.json");

    newSceneWindow->Create = [this, weakWindow, dialogBrowser, saveFolder, fileName](vex::ImGUIWrapper& wrapper) {
        wrapper.addUIFunction([=, this]() {
            auto window = weakWindow.lock(); if (!window || !window->isOpen) return;
            ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Create New Scene", &window->isOpen)) {

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
                                window->isOpen = false;
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
