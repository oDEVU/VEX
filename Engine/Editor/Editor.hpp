#pragma once
#include "Engine.hpp"
#include "EditorCamera.hpp"

#include "editorProperties.hpp"

#include "Tools/EditorMenuBar.hpp"
#include "Tools/AssetBrowser.hpp"

#include <ImGuizmo.h>
#include <nlohmann/json.hpp>

#include <memory>

namespace vex {

    struct SceneRenderData;

    class Editor : public Engine {
    public:
        Editor(const char* title, int width, int height, GameInfo gInfo, const std::string& projectBinaryPath);
        ~Editor(){
        }

        // We override render to inject our Editor UI logic
        void render() override;
        void update(float deltaTime) override;
        void processEvent(const SDL_Event& event, float deltaTime) override;

        void requestSceneReload(const std::string& scenePath);

        std::string getProjectBinaryPath() const { return m_projectBinaryPath; }
        AssetBrowser* getAssetBrowser() const { return m_assetBrowser.get(); }
        const BrowserIcons& GetEditorIcons() const { return m_icons; }

        EditorProperties* getEditorProperties() { return &m_editorProperties; }

        void OnHotReload();

        void refreshForObject() override {
            m_frame = 0;
            m_refresh = true;
        }
    private:
        // Helper to draw the dockspace and viewport window
        void drawEditorLayout(const SceneRenderData& data, glm::uvec2& outNewResolution);

        void drawGizmo(const glm::vec2& viewportPos, const glm::vec2& viewportSize);

        void loadAssetIcons();

        float raySphereIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& sphereCenter, float sphereRadius);
        float rayTriangleIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);

        void ExtractObjectByEntity(entt::entity entity, std::pair<bool, vex::GameObject*>& selectedObject);

        void SaveConfig(const EditorProperties& data, const std::string& filename) {
            std::ofstream file(filename);
            if (file.is_open()) {
                nlohmann::json j = data;
                file << j.dump(4);
            }
        }

        void LoadConfig(EditorProperties& data, const std::string& filename) {
            std::ifstream file(filename);
            if (file.is_open()) {
                try {
                    nlohmann::json j;
                    file >> j;
                    data = j;
                } catch (const std::exception& e) {
                    std::cerr << "JSON parsing error: " << e.what() << std::endl;
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

        int m_fps = 0;

        bool m_refresh = false;
    };

}
