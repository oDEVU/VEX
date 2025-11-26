#pragma once
#include "Engine.hpp"

namespace vex {

    struct SceneRenderData;

    class Editor : public Engine {
    public:
        Editor(const char* title, int width, int height, GameInfo gInfo, const std::string& projectBinaryPath);
        ~Editor();

        // We override render to inject our Editor UI logic
        void render() override;

    private:
        // Helper to draw the dockspace and viewport window
        void drawEditorLayout(const SceneRenderData& data, glm::uvec2& outNewResolution);

        // State to track if the viewport size changed
        glm::uvec2 m_viewportSize = {1280, 720};
    };

}
