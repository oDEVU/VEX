#include "DialogWindow.hpp"
#include "EditorImGUIWrapper.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "../Core/include/components/SceneManager.hpp"
#include "../Core/include/components/ResolutionManager.hpp"
#include "../Core/include/components/InputSystem.hpp"

#include "../Core/src/components/Window.hpp"
#include "../Core/src/components/backends/vulkan/Interface.hpp"
#include "../Core/src/components/backends/vulkan/Renderer.hpp"
#include "../Core/src/components/backends/vulkan/VulkanImGUIWrapper.hpp"

namespace vex {

    DialogWindow::DialogWindow(const char* title, GameInfo gInfo) : Engine(title, 800, 600, gInfo) {
        dummyCamera = std::make_unique<CameraObject>(*this, "DummyCamera");
        //run();
    }

    DialogWindow::~DialogWindow() {
        log("DialogWindow destroyed");
    }

    void DialogWindow::render() {
        auto cameraEntity = getCamera();
        if (cameraEntity == entt::null){
            log("No camera found. Skipping frame.");
            return;
        }

        if(getResolutionMode() != ResolutionMode::NATIVE){
            setResolutionMode(ResolutionMode::NATIVE);
        }

        if(getInputMode() != InputMode::UI){
            setInputMode(InputMode::UI);
        }

        try {
            SceneRenderData renderData{};
            if (!m_interface->getRenderer().beginFrame(m_resolutionManager->getWindowResolution(), renderData)) {
                log("Skipping frame.");
                return;
            }
            renderData.imguiTextureID = m_interface->getRenderer().getImGuiTextureID(*m_imgui);
            m_interface->getRenderer().renderScene(renderData, cameraEntity, m_registry, m_frame);
            m_imgui->beginFrame();
            m_imgui->executeUIFunctions();

            glm::uvec2 newRes = m_resolutionManager->getWindowResolution();
            drawDialogWindowLayout(renderData, newRes);

            m_imgui->endFrame();
            m_interface->getRenderer().composeFrame(renderData, *m_imgui, true);
            m_interface->getRenderer().endFrame(renderData);

        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Dialogwindow render failed");
            handle_exception(e);
        }
    }

    void DialogWindow::drawDialogWindowLayout(const SceneRenderData& data, glm::uvec2& outNewResolution) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DialogWindowkSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MyDialogDockSpace");

        ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_None;
        dockFlags |= ImGuiDockNodeFlags_NoUndocking;

        if (!ImGui::DockBuilderGetNode(dockspace_id)) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);

            ImGui::DockBuilderDockWindow("Dialog", dock_main_id);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockFlags);
        ImGui::End();

        ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace_id);
        if (node) {
            node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        }

        //ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Dialog", nullptr, childFlags);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        outNewResolution = { (uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y };

        ImGui::Text(m_dialogContent.c_str());

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
