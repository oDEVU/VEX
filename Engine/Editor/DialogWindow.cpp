#include "DialogWindow.hpp"
#include "EditorImGUIWrapper.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "../Core/include/components/SceneManager.hpp"
#include "../Core/include/components/ResolutionManager.hpp"
#include "../Core/include/components/InputSystem.hpp"
#include "components/ErrorUtils.hpp"

#include "../Core/src/components/Window.hpp"
#include "../Core/src/components/backends/vulkan/Interface.hpp"
#include "../Core/src/components/backends/vulkan/Renderer.hpp"
#include "../Core/src/components/backends/vulkan/VulkanImGUIWrapper.hpp"

namespace vex {

    DialogWindow::DialogWindow(const char* title, GameInfo gInfo) : Engine(title, 800, 600, gInfo) {
    }

    DialogWindow::~DialogWindow() {
        log("DialogWindow destroyed");
    }

    void DialogWindow::render() {
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
            m_imgui->beginFrame();
            m_imgui->executeUIFunctions();

            glm::uvec2 newRes = m_resolutionManager->getWindowResolution();
            drawDialogWindowLayout(renderData, newRes);

            m_imgui->endFrame();
            m_interface->getRenderer().composeFrame(renderData, *m_imgui, true);
            m_interface->getRenderer().endFrame(renderData);

        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Dialogwindow render failed");
            handle_critical_exception(e);
        }
    }

    void DialogWindow::drawDialogWindowLayout(const SceneRenderData& data, glm::uvec2& outNewResolution) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DialogWindowkSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspaceId = ImGui::GetID("MyDialogDockSpace");

        ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_None;
        dockFlags |= ImGuiDockNodeFlags_NoUndocking;

        if (!ImGui::DockBuilderGetNode(dockspaceId)) {
            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

            ImGuiID dockMainId = dockspaceId;
            ImGuiID dockRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.2f, nullptr, &dockMainId);

            ImGui::DockBuilderDockWindow("Dialog", dockMainId);

            ImGui::DockBuilderFinish(dockspaceId);
        }

        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockFlags);
        ImGui::End();

        ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
        if (node) {
            node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        }

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Dialog", nullptr, childFlags);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        outNewResolution = { (uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y };

        ImGui::Text("%s", m_dialogContent.c_str());

        ImGui::End();
        ImGui::PopStyleVar();
    }
}