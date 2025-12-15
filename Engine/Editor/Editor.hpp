/**
 * @file   Editor.hpp
 * @brief  The main Editor class, inheriting from Engine and providing editor-specific functionality and UI.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "Engine.hpp"
#include "EditorCamera.hpp"

#include "editorProperties.hpp"
#include "projectProperties.hpp"

#include "Tools/EditorMenuBar.hpp"
#include "Tools/AssetBrowser.hpp"

#include <ImGuizmo.h>
#include <nlohmann/json.hpp>

#include <memory>

namespace vex {

    struct SceneRenderData;

    /// @brief The main Editor class, inheriting from Engine and providing editor-specific functionality and UI.
    class Editor : public Engine {
    public:

        /**
        * @brief Constructs the Editor.
        * @param const char* title - The title of the editor window.
        * @param int width - Initial width of the editor window.
        * @param int height - Initial height of the editor window.
        * @param GameInfo gInfo - Game Info class containing version, name etc.
        * @param const std::string& projectBinaryPath - Path to the project's binary directory.
        */
        Editor(const char* title, int width, int height, GameInfo gInfo, const std::string& projectBinaryPath);
        ~Editor(){
        }

        /// @brief Overrides the Engine's render function to inject the Editor UI logic (dockspace, viewport, tools).
        void render() override;

        /// @brief Overrides the Engine's update function to handle editor-specific logic.
        /// @param float deltaTime - Time since last frame.
        void update(float deltaTime) override;

        /// @brief Overrides the Engine's processEvent function to handle editor-specific events (e.g., gizmo interaction, camera movement).
        /// @param const SDL_Event& event - Event to process.
        /// @param float deltaTime - Time since last frame.
        void processEvent(const SDL_Event& event, float deltaTime) override;

        /// @brief Requests a scene reload with a specific path. The reload is typically executed in the main update loop.
        /// @param const std::string& scenePath - The path to the scene file to load.
        void requestSceneReload(const std::string& scenePath);

        /// @brief Gets the path to the project's binary directory.
        /// @return std::string - The project binary path.
        std::string getProjectBinaryPath() const { return m_projectBinaryPath; }

        /// @brief Gets a pointer to the AssetBrowser instance.
        /// @return AssetBrowser* - Pointer to the AssetBrowser.
        AssetBrowser* getAssetBrowser() const { return m_assetBrowser.get(); }

        /// @brief Gets the collection of icons used within the editor's UI.
        /// @return const BrowserIcons& - Reference to the icon collection.
        const BrowserIcons& GetEditorIcons() const { return m_icons; }

        /// @brief Gets a pointer to the mutable editor properties.
        /// @return EditorProperties* - Pointer to the editor properties structure.
        EditorProperties* getEditorProperties() { return &m_editorProperties; }

        /// @brief Gets a pointer to the mutable project properties.
        /// @return ProjectProperties* - Pointer to the project properties structure.
        ProjectProperties* getProjectProperties() { return &m_projectProperties; }

        /// @brief Handler for post-hot-reload operations.
        void OnHotReload();

        /// @brief Overrides the base Engine function to trigger a frame and refresh.
        void refreshForObject() override {
            m_frame = 0;
            m_refresh = true;
        }
    private:
        /**
        * @brief Helper to draw the ImGUI dockspace and viewport window for the editor.
        * @param const SceneRenderData& data - Data required for scene rendering.
        * @param glm::uvec2& outNewResolution - Output parameter for the new viewport resolution if it changed.
        */
        void drawEditorLayout(const SceneRenderData& data, glm::uvec2& outNewResolution);

        /**
                 * @brief Draws the ImGuizmo for the currently selected object.
                 * @param const glm::vec2& viewportPos - The position of the viewport window.
                 * @param const glm::vec2& viewportSize - The size of the viewport window.
                 */
        void drawGizmo(const glm::vec2& viewportPos, const glm::vec2& viewportSize);

        /// @brief Loads the texture icons used for the Asset Browser and other UI elements.
        void loadAssetIcons();

        /**
                 * @brief Performs a ray-sphere intersection test.
                 * @param const glm::vec3& rayOrigin - The origin point of the ray.
                 * @param const glm::vec3& rayDir - The direction vector of the ray.
                 * @param const glm::vec3& sphereCenter - The center point of the sphere.
                 * @param float sphereRadius - The radius of the sphere.
                 * @return float - The distance to the intersection point, or infinity if no intersection.
                 */
        float raySphereIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& sphereCenter, float sphereRadius);

        /**
                 * @brief Performs a ray-triangle intersection test (using the Möller–Trumbore algorithm).
                 * @param const glm::vec3& rayOrigin - The origin point of the ray.
                 * @param const glm::vec3& rayDir - The direction vector of the ray.
                 * @param const glm::vec3& v0 - The first vertex of the triangle.
                 * @param const glm::vec3& v1 - The second vertex of the triangle.
                 * @param const glm::vec3& v2 - The third vertex of the triangle.
                 * @return float - The distance to the intersection point, or infinity if no intersection.
                 */
        float rayTriangleIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);

        /**
                 * @brief Retrieves the GameObject corresponding to a given entity ID and updates the selected object pair.
                 * @param entt::entity entity - The entity ID to look up.
                 * @param std::pair<bool, vex::GameObject*>& selectedObject - The pair to update with the selection status and pointer.
                 */
        void ExtractObjectByEntity(entt::entity entity, std::pair<bool, vex::GameObject*>& selectedObject);

        /**
                 * @brief Saves the current EditorProperties configuration to a JSON file.
                 * @param const EditorProperties& data - The data structure to serialize and save.
                 * @param const std::string& filename - The path/filename of the JSON file.
                 */
        void SaveConfig(const EditorProperties& data, const std::string& filename) {
            std::ofstream file(filename);
            if (file.is_open()) {
                nlohmann::json j = data;
                file << j.dump(4);
            }
        }

        /**
                 * @brief Loads the EditorProperties configuration from a JSON file.
                 * @param EditorProperties& data - The data structure to deserialize into.
                 * @param const std::string& filename - The path/filename of the JSON file.
                 */
        void LoadConfig(EditorProperties& data, const std::string& filename) {
            std::ifstream file(filename);
            if (file.is_open()) {
                try {
                    nlohmann::json j;
                    file >> j;
                    data = j;
                } catch (const std::exception& e) {
                    //std::cerr << "JSON parsing error: " << e.what() << std::endl;
                    vex::handle_exception(e);
                }
            }
        }

        /**
                 * @brief Saves the current ProjectProperties configuration to a JSON file.
                 * @param const ProjectProperties& data - The data structure to serialize and save.
                 * @param const std::string& filename - The path/filename of the JSON file.
                 */
        void SaveProjectConfig(const ProjectProperties& data, const std::string& filename) {
            std::ofstream file(filename);
            if (file.is_open()) {
                nlohmann::json j = data;
                file << j.dump(4);
            }
        }

        /**
                 * @brief Loads the ProjectProperties configuration from a JSON file.
                 * @param ProjectProperties& data - The data structure to deserialize into.
                 * @param const std::string& filename - The path/filename of the JSON file.
                 */
        void LoadProjectConfig(ProjectProperties& data, const std::string& filename) {
            std::ifstream file(filename);
            if (file.is_open()) {
                try {
                    nlohmann::json j;
                    file >> j;
                    data = j;
                } catch (const std::exception& e) {
                    //std::cerr << "JSON parsing error: " << e.what() << std::endl;
                    vex::handle_exception(e);
                }
            }
        }

        // State to track if the viewport size changed
        glm::uvec2 m_viewportSize = {1280, 720};
        std::unique_ptr<EditorCameraObject> m_camera;
        std::unique_ptr<EditorMenuBar> m_editorMenuBar;
        std::pair<bool, GameObject*> m_selectedObject = {false, nullptr};
        std::unique_ptr<AssetBrowser> m_assetBrowser;
        BrowserIcons m_icons;

        std::string m_pendingSceneToLoad = "";
        bool m_waitForGui = true;

        std::string m_projectBinaryPath;

        ImGuizmo::OPERATION m_currentGizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE m_currentGizmoMode = ImGuizmo::WORLD;
        bool m_useSnap = false;
        float m_snapValue[3] = { 0.5f, 0.5f, 0.5f };

        bool m_isHoveringGizmoUI = false;

        EditorProperties m_editorProperties;
        EditorProperties m_SavedEditorProperties;

        ProjectProperties m_projectProperties;
        ProjectProperties m_SavedProjectProperties;

        int m_fps = 0;

        bool m_refresh = false;
    };

}
