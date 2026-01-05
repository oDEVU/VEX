/**
 * @file   ProjectSelector.hpp
 * @brief  Specialized Engine class for the initial project selection screen.
 * @author Eryk Roszkowski
 ***********************************************/

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

#include <cstdint>
#include <vector>
#include <string>

namespace vex {

    /// @brief Metadata structure for a recently used project.
    struct ProjectMetadata {
        std::string name;
        std::string path;
        std::string version;
    };

    /// @brief Specialized Engine class for the initial project selection screen.
    class ProjectSelector : public Engine {
    public:
        /**
             * @brief Constructs the ProjectSelector window.
             * @param const char* title - The title of the window.
             */
        ProjectSelector(const char* title);
        ~ProjectSelector() = default;

        /// @brief Overrides the base render function to draw the project selection UI.
        void render() override;

        /// @brief Gets the path of the project selected by the user.
        /// @return std::string - The path to the selected project.
        std::string getSelectedProject() const { return m_selectedProjectPath; }

    private:
        /**
             * @brief Internal helper to draw the ImGUI layout for the project selector.
             * @param const SceneRenderData& data - Data required for scene rendering (unused here).
             */
        void drawSelectorLayout(const SceneRenderData& data);

        /// @brief Loads the list of recently known projects from the configuration file.
        void loadKnownProjects();

        /// @brief Saves the current list of known projects to the configuration file.
        void saveKnownProjects();

        /**
                 * @brief Scans a directory path for a valid VEX project file and adds it to the list.
                 * @param const std::string& path - The directory path to scan.
                 */
        void scanAndAddProject(const std::string& path);

        /**
                 * @brief Sets the path of the project that the user selected and triggers the closure of the selector.
                 * @param const std::string& projectPath - The path of the project selected.
                 */
        void selectProject(const std::string& projectPath);

        /**
         * @brief Draws the modal for creating a new project.
         */
        void drawCreatorModal();

        /**
         * @brief Draws the modal for adding an existing project.
         */
        void drawAddExistingModal();

        /**
         * @brief Draws the modal for moving an existing project.
         */
        void drawMoveProjectModal();

        /**
         * @brief Creates a new project with the given name and path.
         */
        void createNewProject();

        /**
         * @brief Removes a project from the list.
         */
        void removeProject(int index);

        /**
         * @brief Moves a project to a new parent directory.
         */
        void moveProject(int index, const std::string& newParentDir);

        /**
         * @brief Gets the path to the user's documents directory.
         */
        std::filesystem::path getUserDocumentsDir();

        char m_newProjName[64] = "NewVexProject";
        char m_newProjParentDir[512] = "";

        char m_addExistingBuffer[512] = "";
        char m_moveProjDestBuffer[512] = "";

        int m_contextMenuTargetIndex = -1;

        std::vector<ProjectMetadata> m_projects;
        std::string m_configPath = "recent_projects.json";
        char m_inputBuffer[256] = "";

        std::string m_selectedProjectPath = "";

        bool shouldClose = false;
        uint8_t closeDelay = 5;
        bool m_showCreatorModal = false;
        bool m_showAddExistingModal = false;
        bool m_showMoveModal = false;
    };
}
