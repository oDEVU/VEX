#pragma once

#include "Engine.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/SceneManager.hpp"
#include "components/ResolutionManager.hpp"
#include "components/InputSystem.hpp"
#include "components/Window.hpp"

#include "../../Core/src/components/backends/vulkan/Interface.hpp"
#include "../../Core/src/components/backends/vulkan/Renderer.hpp"
#include "../../Core/src/components/backends/vulkan/VulkanImGUIWrapper.hpp"

#include <vector>
#include <string>

namespace vex {

    struct ProjectMetadata {
        std::string name;
        std::string path;
        std::string version;
    };

    class ProjectSelector : public Engine {
    public:
        ProjectSelector(const char* title);
        ~ProjectSelector() = default;

        void render() override;

    private:
        void drawSelectorLayout(const SceneRenderData& data);

        void loadKnownProjects();
        void saveKnownProjects();
        void scanAndAddProject(const std::string& path);
        void launchEditor(const std::string& projectPath);

        std::unique_ptr<CameraObject> m_dummyCamera;

        std::vector<ProjectMetadata> m_projects;
        std::string m_configPath = "recent_projects.json";

        char m_inputBuffer[256] = "";
    };
}
