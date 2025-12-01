#include "Editor.hpp"
#include "EditorImGUIWrapper.hpp"
#include "Tools/PropertiesMenu.hpp"
#include "Tools/SceneMenu.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/glm.hpp>
#include <volk.h>

#include "components/GameComponents/BasicComponents.hpp"
#include "ImReflect.hpp"

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
        m_imgui = std::make_unique<EditorImGUIWrapper>(m_window->GetSDLWindow(), *m_interface->getContext());
        m_imgui->init();

        log("Initializing engine components...");

        m_inputSystem = std::make_unique<InputSystem>(m_registry, m_window->GetSDLWindow());
        m_physicsSystem = std::make_unique<PhysicsSystem>(m_registry);
        m_physicsSystem->init();

        m_sceneManager = std::make_unique<SceneManager>();
        setInputMode(InputMode::UI);

        log("Initializing editor components...");

        std::filesystem::path assetsPath = projectBinaryPath;
        assetsPath = assetsPath / "Assets";
        m_assetBrowser = std::make_unique<AssetBrowser>(assetsPath.string());
        loadAssetIcons();

        m_projectBinaryPath = projectBinaryPath;

        m_camera = std::make_unique<EditorCameraObject>(*this, "VexEditorCamera", m_window->GetSDLWindow());
        m_editorMenuBar = std::make_unique<EditorMenuBar>(*m_imgui, *this);

        log("Editor initialized successfully");
    }

    void Editor::update(float deltaTime) {
        if (!m_pendingSceneToLoad.empty() && !m_waitForGui) {
            m_interface->WaitForGPUToFinish();
            getSceneManager()->loadScene(m_pendingSceneToLoad, *this);
            m_pendingSceneToLoad.clear();
            m_waitForGui = true;
            m_frame = 0;
        }else if(!m_pendingSceneToLoad.empty() && m_waitForGui){
            m_waitForGui = false;
        }

        m_camera->Update(deltaTime);
        render();
        m_frame = 1;
    }

    void Editor::processEvent(const SDL_Event& event, float deltaTime) {
        m_camera->processEvent(event, deltaTime);
    }

    void Editor::render() {
        auto cameraEntity = m_camera->GetEntity();
        if (cameraEntity == entt::null) return;

        if(getResolutionMode() != ResolutionMode::NATIVE){
            setResolutionMode(ResolutionMode::NATIVE);
        }

        if(getInputMode() != InputMode::UI){
            setInputMode(InputMode::UI);
        }

        glm::uvec2 viewportRes = (m_viewportSize.x > 0 && m_viewportSize.y > 0)
                                         ? m_viewportSize
                                         : m_resolutionManager->getRenderResolution();

        try {
            SceneRenderData renderData{};
            if (!m_interface->getRenderer().beginFrame(viewportRes, renderData)) {
                return;
            }
            renderData.imguiTextureID = m_interface->getRenderer().getImGuiTextureID(*m_imgui);
            m_interface->getRenderer().renderScene(renderData, cameraEntity, m_registry, m_frame, true);
            m_imgui->beginFrame();
            m_imgui->executeUIFunctions();

            glm::uvec2 newRes = viewportRes;
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

    void Editor::requestSceneReload(const std::string& scenePath){
        m_pendingSceneToLoad = scenePath;
    }

    void Editor::loadAssetIcons() {
        auto* resources = m_interface->getResources();
        auto* vfs = m_vfs.get();
        auto* vulkanGUI = static_cast<VulkanImGUIWrapper*>(m_imgui.get());
            if (!vulkanGUI) {
                log("Failed to cast ImGUI wrapper to VulkanImGUIWrapper");
                return;
            }
        std::vector<std::pair<std::string, ImTextureID*>> targets = {
            { "../Assets/icons/folder-fill.png", &m_icons.folder },
            { "../Assets/icons/file-fill.png",   &m_icons.file },
            { "../Assets/icons/box-3-fill.png",   &m_icons.mesh },
            { "../Assets/icons/file-image-fill.png",   &m_icons.texture },
            { "../Assets/icons/map-2-fill.png",   &m_icons.scene },
            { "../Assets/icons/file-text-fill.png",   &m_icons.text },
            { "../Assets/icons/window-fill.png",   &m_icons.ui }
        };

        VkSampler sampler = resources->getTextureSampler();

        for (auto& [path, targetPtr] : targets) {
            std::string name = "editor_" + path; // Unique name for resource manager

            try {
                std::filesystem::path correctPath = GetExecutableDir() / path;
                resources->loadTexture(correctPath.string(), name, vfs);

                VkDescriptorSet ds = vulkanGUI->addTexture(
                    sampler,
                    resources->getTextureView(name),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                );

                *targetPtr = (ImTextureID)ds;

            } catch (const std::exception& e) {
                log("Failed to load icon: %s", path.c_str());
                VkDescriptorSet ds = vulkanGUI->addTexture(
                    sampler,
                    resources->getTextureView("default"),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                );
                *targetPtr = (ImTextureID)ds;
            }
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
                                        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
                                        ImGuiWindowFlags_MenuBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("EditorDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        m_editorMenuBar->DrawBar();

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

        ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_None;
        dockFlags |= ImGuiDockNodeFlags_NoUndocking;
        dockFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;

        if (!ImGui::DockBuilderGetNode(dockspace_id)) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
            ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.4f, nullptr, &dock_main_id);
            ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.4f, nullptr, &dock_main_id);
            ImGuiID dock_right_top_id, dock_right_bottom_id;
            ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.5f, &dock_right_bottom_id, &dock_right_top_id);

            ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
            ImGui::DockBuilderDockWindow("Inspector", dock_left_id);
            ImGui::DockBuilderDockWindow("Assets", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Scene", dock_right_top_id);
            ImGui::DockBuilderDockWindow("Properties", dock_right_bottom_id);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockFlags);
        ImGui::End();

        ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr, childFlags);

        if (m_camera) {
            m_camera->setViewportHovered(ImGui::IsWindowHovered());
        }

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        outNewResolution = { (uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y };

        if (data.imguiTextureID) {
            ImGui::Image((ImTextureID)data.imguiTextureID, viewportPanelSize);
        }else {
            ImGui::Text("Viewport Texture Could not be retrieved.");
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Inspector", nullptr, childFlags);
        ImGui::Text("[Game Objects]");
        ImGui::End();

        ImGui::Begin("Assets", nullptr, childFlags);

        if (m_assetBrowser) {
                std::string openedFile = m_assetBrowser->Draw(m_icons);

                if (!openedFile.empty()) {
                    log("Opening file: %s", openedFile.c_str());
                    if (m_assetBrowser->GetExtension(openedFile) == ".json") {
                        if (m_assetBrowser->GetJSONAssetType(openedFile) == 1){
                            requestSceneReload(openedFile);
                        }
                    }
                }
            }else{
                ImGui::Text("Could not load assets");
            }
        ImGui::End();

        ImGui::Begin("Scene", nullptr, childFlags);
            DrawSceneHierarchy(*this, m_selectedObject);
        ImGui::End();

        ImGui::Begin("Properties", nullptr, childFlags);
            if(m_selectedObject.second){
                DrawPropertiesOfAnObject(m_selectedObject.second, m_selectedObject.first);
            }
        ImGui::End();

        if (!m_pendingSceneToLoad.empty() || m_pendingSceneToLoad != "") {

            if (ImGui::Begin("Opening scene...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Loading scene: %s", m_pendingSceneToLoad.c_str());
            }
            ImGui::End();
        }
    }
}
