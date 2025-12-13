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
        ImGui::Spacing();

        float cardWidth = 220.0f;
        int columns = static_cast<int>(viewport->Size.x / cardWidth);
        if (columns < 1) columns = 1;

        if (ImGui::BeginTable("ProjectsGrid", columns)) {
            if(m_projects.size() < 1){
                ImGui::TableNextColumn();
                ImGui::Text("No projects yet.");
            }
            for (const auto& proj : m_projects) {
                ImGui::TableNextColumn();

                ImGui::PushID(proj.path.c_str());
                if (ImGui::Button(proj.name.c_str(), ImVec2(200, 300))) {
                    selectProject(proj.path);
                }

                float wrapPosX = ImGui::GetCursorPosX() + (cardWidth - 5.f);
                ImGui::PushTextWrapPos(wrapPosX);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", proj.path.c_str());
                ImGui::PopTextWrapPos();

                ImGui::Text("Ver: %s", proj.version.c_str());
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Separator();

        ImGui::Text("Add Existing Project");
        ImGui::SetNextItemWidth(400);
        ImGui::InputTextWithHint("##pathInput", "Paste absolute project folder path...", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Add Project")) {
            scanAndAddProject(m_inputBuffer);
            memset(m_inputBuffer, 0, 256);
        }

        ImGui::End();
        ImGui::PopStyleVar(2);
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
}
