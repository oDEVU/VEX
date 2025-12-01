#pragma once
#include "Engine.hpp"
#include "EditorCamera.hpp"

#include "Tools/EditorMenuBar.hpp"
#include "Tools/AssetBrowser.hpp"

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

    private:
        // Helper to draw the dockspace and viewport window
        void drawEditorLayout(const SceneRenderData& data, glm::uvec2& outNewResolution);

        void loadAssetIcons();

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
    };

}
