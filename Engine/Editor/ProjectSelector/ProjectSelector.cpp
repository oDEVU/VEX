#include "ProjectSelector.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstdlib>

#include "components/PhysicsSystem.hpp"
#include "entt/entity/entity.hpp"

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

namespace vex {

    ProjectSelector::ProjectSelector(const char* title)
        : Engine(title, 800, 600, GameInfo{"Vex Selector", 1, 0, 0})
    {
        loadKnownProjects();

        std::filesystem::path docs = getUserDocumentsDir();
        std::filesystem::path defaultSave = docs / "VexProjects";

        std::string pathStr = defaultSave.string();
        if (pathStr.length() < sizeof(m_newProjParentDir)) {
            strcpy(m_newProjParentDir, pathStr.c_str());
        }
    }

    void ProjectSelector::render() {
        if(shouldClose){
            if(closeDelay > 0){
                closeDelay--;
            }else{
                m_running = false;
            }
        }else{

        if(getResolutionMode() != ResolutionMode::NATIVE){
            setResolutionMode(ResolutionMode::NATIVE);
        }

        if(getInputMode() != InputMode::UI){
            setInputMode(InputMode::UI);
        }

        try {
            SceneRenderData renderData{};
            if (!m_interface->getRenderer().beginFrame(m_resolutionManager->getWindowResolution(), renderData)) {
                return;
            }

            renderData.imguiTextureID = m_interface->getRenderer().getImGuiTextureID(*m_imgui);

            m_imgui->beginFrame();
            m_imgui->executeUIFunctions();

            drawSelectorLayout(renderData);
            drawCreatorModal();
            drawAddExistingModal();
            drawMoveProjectModal();

            m_imgui->endFrame();

            m_interface->getRenderer().composeFrame(renderData, *m_imgui, true);
            m_interface->getRenderer().endFrame(renderData);

        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Selector render failed: %s", e.what());
        }
        }
    }

    void ProjectSelector::drawSelectorLayout(const SceneRenderData& data) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

        ImGui::Begin("SelectorSpace", nullptr, window_flags);

        ImGui::Text("VEX ENGINE - PROJECTS");
        ImGui::Separator();

        if (ImGui::Button("+ Create New Project", ImVec2(200, 30))) {
            m_showCreatorModal = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Add Existing Project", ImVec2(200, 30))) {
            m_showAddExistingModal = true;
            memset(m_addExistingBuffer, 0, sizeof(m_addExistingBuffer));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        float cardWidth = 220.0f;
        int columns = static_cast<int>(viewport->Size.x / cardWidth);
        if (columns < 1) columns = 1;

        if (ImGui::BeginTable("ProjectsGrid", columns)) {
            if(m_projects.size() < 1){
                ImGui::TableNextColumn();
                ImGui::Text("No projects yet.");
            }
            for (int i = 0; i < m_projects.size(); ++i) {
                ImGui::TableNextColumn();

                ImGui::PushID(m_projects[i].path.c_str());
                if (ImGui::Button(m_projects[i].name.c_str(), ImVec2(200, 300))) {
                    selectProject(m_projects[i].path);
                }

                if (ImGui::BeginPopupContextItem()) {
                    ImGui::TextDisabled("%s", m_projects[i].name.c_str());
                    ImGui::Separator();

                    if (ImGui::MenuItem("Move Project Folder...")) {
                        m_contextMenuTargetIndex = i;
                        m_showMoveModal = true;

                        std::filesystem::path p(m_projects[i].path);
                        std::string parent = p.parent_path().string();
                        strcpy(m_moveProjDestBuffer, parent.c_str());
                    }

                    if (ImGui::MenuItem("Remove from List")) {
                        removeProject(i);
                        ImGui::EndPopup();
                        ImGui::PopID();
                        ImGui::EndTable();
                        goto EndGrid;
                    }

                    ImGui::EndPopup();
                }

                float wrapPosX = ImGui::GetCursorPosX() + (cardWidth - 5.f);
                ImGui::PushTextWrapPos(wrapPosX);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", m_projects[i].path.c_str());
                ImGui::PopTextWrapPos();

                ImGui::Text("Ver: %s", m_projects[i].version.c_str());
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

    EndGrid:
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void ProjectSelector::drawAddExistingModal() {
            if (m_showAddExistingModal) ImGui::OpenPopup("Add Existing Project");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Add Existing Project", &m_showAddExistingModal, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Enter absolute path to project folder:");
                #ifdef _WIN32
                ImGui::InputTextWithHint("##pathInput", "C:/Projects/MyProject", m_addExistingBuffer, IM_ARRAYSIZE(m_addExistingBuffer));
                #else
                ImGui::InputTextWithHint("##pathInput", "/home/user/Projects/MyProject", m_addExistingBuffer, IM_ARRAYSIZE(m_addExistingBuffer));
                #endif

                ImGui::Dummy(ImVec2(0, 10));

                if (ImGui::Button("Add", ImVec2(100, 0))) {
                    scanAndAddProject(m_addExistingBuffer);
                    m_showAddExistingModal = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                    m_showAddExistingModal = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        void ProjectSelector::drawMoveProjectModal() {
            if (m_showMoveModal) ImGui::OpenPopup("Move Project");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Move Project", &m_showMoveModal, ImGuiWindowFlags_AlwaysAutoResize)) {

                if (m_contextMenuTargetIndex >= 0 && m_contextMenuTargetIndex < m_projects.size()) {
                    const auto& proj = m_projects[m_contextMenuTargetIndex];
                    ImGui::Text("Moving project: '%s'", proj.name.c_str());
                    ImGui::TextDisabled("Current: %s", proj.path.c_str());
                    ImGui::Separator();

                    ImGui::Text("New Parent Directory:");
                    ImGui::InputText("##MoveDir", m_moveProjDestBuffer, IM_ARRAYSIZE(m_moveProjDestBuffer));

                    ImGui::Dummy(ImVec2(0, 15));

                    if (ImGui::Button("Move Folder", ImVec2(120, 0))) {
                        moveProject(m_contextMenuTargetIndex, m_moveProjDestBuffer);
                        m_showMoveModal = false;
                        ImGui::CloseCurrentPopup();
                    }
                } else {
                    m_showMoveModal = false;
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    m_showMoveModal = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        void ProjectSelector::removeProject(int index) {
                if (index >= 0 && index < m_projects.size()) {
                    log("Removing project from list: %s", m_projects[index].name.c_str());
                    m_projects.erase(m_projects.begin() + index);
                    saveKnownProjects();
                }
            }

            void ProjectSelector::moveProject(int index, const std::string& newParentDir) {
                if (index < 0 || index >= m_projects.size()) return;

                auto& proj = m_projects[index];
                std::filesystem::path oldPath(proj.path);
                std::filesystem::path newParent(newParentDir);
                std::filesystem::path newPath = newParent / oldPath.filename();

                if (!std::filesystem::exists(oldPath)) {
                    log(LogLevel::ERROR, "Source path does not exist!");
                    return;
                }
                if (std::filesystem::exists(newPath)) {
                    log(LogLevel::ERROR, "Destination already exists: %s", newPath.string().c_str());
                    return;
                }

                try {
                    if (!std::filesystem::exists(newParent)) std::filesystem::create_directories(newParent);

                    std::filesystem::rename(oldPath, newPath);
                    log("Project moved to: %s", newPath.string().c_str());

                    proj.path = newPath.string();

                    saveKnownProjects();

                } catch (const std::exception& e) {
                    log(LogLevel::ERROR, "Failed to move project: %s", e.what());
                }
            }

    void ProjectSelector::loadKnownProjects() {
        if (!std::filesystem::exists(m_configPath)) return;

        try {
            std::ifstream file(m_configPath);
            nlohmann::json j;
            file >> j;

            for (const auto& item : j) {
                scanAndAddProject(item["path"].get<std::string>());
            }
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Failed to load projects: %s", e.what());
        }
    }

    void ProjectSelector::saveKnownProjects() {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& p : m_projects) {
            j.push_back({
                {"name", p.name},
                {"path", p.path},
                {"version", p.version}
            });
        }
        std::ofstream file(m_configPath);
        file << j.dump(4);
    }

    void ProjectSelector::scanAndAddProject(const std::string& pathStr) {
        std::filesystem::path path(pathStr);
        std::filesystem::path projectFile = path / "VexProject.json";

        if (std::filesystem::exists(projectFile)) {
            try {
                std::ifstream f(projectFile);
                nlohmann::json j;
                f >> j;

                std::string pName = j.value("project_name", "Unknown Project");
                std::string pVer  = j.value("version", "1");

                for(auto& p : m_projects) {
                    if(p.path == pathStr) return;
                }

                m_projects.push_back({pName, pathStr, pVer});
                saveKnownProjects();
                log("Added project: %s", pName.c_str());

            } catch(const std::exception& e) {
                log(LogLevel::ERROR, "Invalid VexProject.json at %s: %s", pathStr.c_str(), e.what());
            }
        } else {
            log(LogLevel::ERROR, "No VexProject.json found at %s", pathStr.c_str());
        }
    }

    void ProjectSelector::selectProject(const std::string& projectPath) {
        log("Project selected: %s", projectPath.c_str());
        m_selectedProjectPath = projectPath;
        shouldClose = true;
    }

    void ProjectSelector::drawCreatorModal() {
            if (m_showCreatorModal) {
                ImGui::OpenPopup("Create New Project");
            }

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Create New Project", &m_showCreatorModal, ImGuiWindowFlags_AlwaysAutoResize)) {

                ImGui::Text("Project Name:");
                ImGui::InputText("##NewProjName", m_newProjName, IM_ARRAYSIZE(m_newProjName));

                ImGui::Spacing();

                ImGui::Text("Location:");
                ImGui::InputText("##NewProjDir", m_newProjParentDir, IM_ARRAYSIZE(m_newProjParentDir));
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("The folder where your project folder will be created.\nDefault is Documents/VexProjects.");

                ImGui::Dummy(ImVec2(0, 15));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 5));

                if (ImGui::Button("Create", ImVec2(120, 0))) {
                    createNewProject();
                    m_showCreatorModal = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    m_showCreatorModal = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        void ProjectSelector::createNewProject() {
            std::string name(m_newProjName);
            std::string parent(m_newProjParentDir);

            if (name.empty() || parent.empty()) return;

            std::filesystem::path exeDir = GetExecutableDir();
            std::filesystem::path templatePath = exeDir / ".." / "Assets" / "default" / "NewProject";

            if (!std::filesystem::exists(templatePath)) {
                log(LogLevel::ERROR, "Missing template: %s", templatePath.string().c_str());
                return;
            }

            std::filesystem::path destParent(parent);
            std::filesystem::path newProjectPath = destParent / name;

            if (std::filesystem::exists(newProjectPath)) {
                log(LogLevel::ERROR, "Project already exists at: %s", newProjectPath.string().c_str());
                return;
            }

            try {
                if (!std::filesystem::exists(destParent)) {
                    std::filesystem::create_directories(destParent);
                }

                std::filesystem::copy(templatePath, newProjectPath, std::filesystem::copy_options::recursive);

                std::filesystem::path jsonPath = newProjectPath / "VexProject.json";
                if (std::filesystem::exists(jsonPath)) {
                    std::ifstream i(jsonPath);
                    nlohmann::json j;
                    i >> j;

                    j["project_name"] = name;

                    std::ofstream o(jsonPath);
                    o << j.dump(4);
                }

                scanAndAddProject(newProjectPath.string());
                log("Created project: %s", name.c_str());

            } catch (const std::exception& e) {
                log(LogLevel::ERROR, "Creation failed: %s", e.what());
            }
        }

        std::filesystem::path ProjectSelector::getUserDocumentsDir() {
        #ifdef _WIN32
                CHAR path[MAX_PATH];
                HRESULT result = SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, path);
                if (SUCCEEDED(result)) {
                    return std::filesystem::path(path);
                }
        #else
                const char* homeDir = getenv("HOME");
                if (homeDir) {
                    return std::filesystem::path(homeDir) / "Documents";
                }
        #endif
                return std::filesystem::current_path();
            }
}
