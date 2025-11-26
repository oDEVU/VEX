#include "Editor.hpp"
#include <imgui.h>

#include "../Core/include/components/SceneManager.hpp"
#include "../Core/include/components/ResolutionManager.hpp"
#include "../Core/include/components/InputSystem.hpp"

#include "../Core/src/components/Window.hpp"
#include "../Core/src/components/backends/vulkan/Interface.hpp"
#include "../Core/src/components/backends/vulkan/Renderer.hpp"
#include "../Core/src/components/backends/vulkan/VulkanImGUIWrapper.hpp"

namespace vex {

    Editor::Editor(const char* title, int width, int height, GameInfo gInfo, const std::string& projectBinaryPath) : Engine(SkipInit{}) {
        std::filesystem::current_path(GetExecutableDir());
        SetAssetRoot(projectBinaryPath);
        m_gameInfo = gInfo;
        if(m_gameInfo.versionMajor == 0 && m_gameInfo.versionMinor == 0 && m_gameInfo.versionPatch == 0){
            log("Project version not set!");
        }

        log("Creating window..");
        m_window = std::make_shared<Window>(title, width, height);
        m_resolutionManager = std::make_unique<ResolutionManager>(m_window->GetSDLWindow());

        log("Initializing virtual file system..\nSwitching vfs path to: %s", projectBinaryPath.c_str());
        m_vfs = std::make_shared<VirtualFileSystem>();
        m_vfs->initialize(projectBinaryPath);

        auto renderRes = m_resolutionManager->getRenderResolution();
        log("Initializing Vulkan interface...");
        m_interface = std::make_unique<Interface>(m_window->GetSDLWindow(), renderRes, m_gameInfo, m_vfs.get());
        m_imgui = std::make_unique<VulkanImGUIWrapper>(m_window->GetSDLWindow(), *m_interface->getContext());
        m_imgui->init();

        log("Initializing engine components...");

        m_inputSystem = std::make_unique<InputSystem>(m_registry, m_window->GetSDLWindow());
        m_physicsSystem = std::make_unique<PhysicsSystem>(m_registry);
        m_physicsSystem->init();

        m_sceneManager = std::make_unique<SceneManager>();
        setInputMode(InputMode::UI);
        log("Editor initialized successfully");
    }

    Editor::~Editor() {

    }

    void Editor::render() {
        auto cameraEntity = getCamera();
        if (cameraEntity == entt::null) return;

        if(getResolutionMode() != ResolutionMode::NATIVE){
            setResolutionMode(ResolutionMode::NATIVE);
        }
        glm::uvec2 currentRes = m_resolutionManager->getRenderResolution();

        try {
            SceneRenderData renderData;
            if (!m_interface->getRenderer().beginFrame(currentRes, renderData)) {
                return;
            }
            m_interface->getRenderer().renderScene(renderData, cameraEntity, m_registry, m_frame);
            m_imgui->beginFrame();
            m_imgui->executeUIFunctions();

            glm::uvec2 newRes = currentRes;
            drawEditorLayout(renderData, newRes);

            if (newRes != m_viewportSize && newRes.x > 0 && newRes.y > 0) {
                m_viewportSize = newRes;
            }

            m_imgui->endFrame();
            m_interface->getRenderer().composeFrame(renderData, *m_imgui, true);
            m_interface->getRenderer().endFrame(renderData);

        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Editor render failed");
            handle_exception(e);
        }
    }

    void Editor::drawEditorLayout(const SceneRenderData& data, glm::uvec2& outNewResolution) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("EditorDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        outNewResolution = { (uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y };

        if (data.imguiTextureID) {
            ImGui::Image((ImTextureID)data.imguiTextureID, viewportPanelSize);
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Inspector");
        ImGui::Text("[Game Objects]");
        ImGui::End();

        ImGui::Begin("Scene Hierarchy");
        ImGui::Text("[Game Objects]");
        ImGui::End();
    }
}
