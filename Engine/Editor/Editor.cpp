#include "Editor.hpp"
#include "EditorImGUIWrapper.hpp"

#include "Tools/PropertiesMenu.hpp"
#include "Tools/worldSettingsMenu.hpp"
#include "Tools/SceneMenu.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
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

        getInterface()->getMeshManager().init(static_cast<Engine*>(this));
        setInputMode(InputMode::UI);

        log("Initializing editor components...");

        std::filesystem::path assetsPath = projectBinaryPath;
        assetsPath = assetsPath / "Assets";
        m_assetBrowser = std::make_unique<AssetBrowser>(assetsPath.string());
        loadAssetIcons();

        m_projectBinaryPath = projectBinaryPath;

        m_camera = std::make_unique<EditorCameraObject>(*this, "VexEditorCamera", m_window->GetSDLWindow());
        m_editorMenuBar = std::make_unique<EditorMenuBar>(*m_imgui, *this);

        LoadConfig(m_editorProperties, "editor_config.json");
        m_SavedEditorProperties = m_editorProperties;

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

        m_fps = static_cast<int>(1.0f / deltaTime);

        m_assetBrowser->setThumbnailSize(m_editorProperties.assetBrowserThumbnailSize);
        m_camera->GetComponent<vex::CameraComponent>().fov = m_editorProperties.editorCameraFov;
        m_camera->GetComponent<vex::CameraComponent>().farPlane = m_editorProperties.editorCameraRenderDistance;

        if(m_SavedEditorProperties != m_editorProperties){
            SaveConfig(m_editorProperties, "editor_config.json");
            m_SavedEditorProperties = m_editorProperties;
        }

        m_camera->Update(deltaTime);
        render();
        if(!m_refresh){
            m_frame = 1;
        }else{
            m_refresh = false;
        }
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
        m_selectedObject.first = false;
        m_selectedObject.second = nullptr;
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
            std::string name = "editor_" + path;

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

    void Editor::drawGizmo(const glm::vec2& viewportPos, const glm::vec2& viewportSize) {
        m_isHoveringGizmoUI = false;

        auto selectedEntity = m_selectedObject.second ? m_selectedObject.second->GetEntity() : entt::null;
        if (selectedEntity == entt::null || !m_registry.all_of<TransformComponent>(selectedEntity)) return;

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);


        auto& camTC = m_registry.get<TransformComponent>(m_camera->GetEntity());
        auto& camComp = m_registry.get<CameraComponent>(m_camera->GetEntity());

        glm::mat4 view = glm::lookAt(
            camTC.getWorldPosition(),
            camTC.getWorldPosition() + camTC.getForwardVector(),
            camTC.getUpVector()
        );

        float aspect = viewportSize.x / viewportSize.y;
        glm::mat4 proj = glm::perspective(glm::radians(camComp.fov), aspect, camComp.nearPlane, camComp.farPlane);

        auto& tc = m_registry.get<TransformComponent>(selectedEntity);
        glm::mat4 transformMatrix = tc.matrix();

        ImGui::SetCursorScreenPos(ImVec2(viewportPos.x + 10, viewportPos.y + 10));

        auto GizmoButton = [&](const char* label, ImGuizmo::OPERATION op) {
            bool wasActive = (m_currentGizmoOperation == op);
            if (wasActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.00f, 0.23f, 0.01f, 1.0f));
            if (ImGui::Button(label)) m_currentGizmoOperation = op;
            if (wasActive) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) m_isHoveringGizmoUI = true;
            ImGui::SameLine();
        };

        GizmoButton("T", ImGuizmo::TRANSLATE);
        GizmoButton("R", ImGuizmo::ROTATE);
        GizmoButton("S", ImGuizmo::SCALE);

        ImGui::Dummy(ImVec2(10, 0)); ImGui::SameLine();

        if(m_currentGizmoMode == ImGuizmo::WORLD) {
            if(ImGui::Button("World")) m_currentGizmoMode = ImGuizmo::LOCAL;
        } else {
            if(ImGui::Button("Local")) m_currentGizmoMode = ImGuizmo::WORLD;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)) m_useSnap = true;
        else if (ImGui::IsKeyReleased(ImGuiKey_LeftCtrl)) m_useSnap = false;

        float snap[3] = { 0.f, 0.f, 0.f };
        if(m_useSnap) {
            if (m_currentGizmoOperation == ImGuizmo::TRANSLATE) { snap[0] = snap[1] = snap[2] = 0.5f; }
            else if (m_currentGizmoOperation == ImGuizmo::ROTATE) { snap[0] = 45.0f; }
            else if (m_currentGizmoOperation == ImGuizmo::SCALE) { snap[0] = snap[1] = snap[2] = 0.1f; }
        }

        bool manipulated = ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            m_currentGizmoOperation,
            m_currentGizmoMode,
            glm::value_ptr(transformMatrix),
            nullptr,
            m_useSnap ? snap : nullptr
        );

        if (manipulated) {
            glm::mat4 localMatrix = transformMatrix;

            entt::entity parent = tc.getParent();
            if (parent != entt::null && m_registry.valid(parent) && m_registry.all_of<TransformComponent>(parent)) {
                glm::mat4 parentMatrix = m_registry.get<TransformComponent>(parent).matrix();
                localMatrix = glm::inverse(parentMatrix) * transformMatrix;
            }

            glm::vec3 translation, scale, skew;
            glm::quat rotation;
            glm::vec4 perspective;
            glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);

            tc.setLocalPosition(translation);
            tc.setLocalScale(scale);

            glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));
            tc.setLocalRotation(eulerRot);

            tc.enableLastTransformed();
        }
    }

    float Editor::raySphereIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& sphereCenter, float sphereRadius) {
        glm::vec3 L = sphereCenter - rayOrigin;
        float tca = glm::dot(L, rayDir);
        if (tca < 0) return -1.0f;
        float d2 = glm::dot(L, L) - tca * tca;
        float radius2 = sphereRadius * sphereRadius;
        if (d2 > radius2) return -1.0f;
        float thc = sqrt(radius2 - d2);
        return tca - thc;
    }

    float Editor::rayTriangleIntersect(
        const glm::vec3& rayOrigin, const glm::vec3& rayDir,
        const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
    {
        const float EPSILON = 0.0000001f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(rayDir, edge2);
        float a = glm::dot(edge1, h);

        if (a > -EPSILON && a < EPSILON) return -1.0f;

        float f = 1.0f / a;
        glm::vec3 s = rayOrigin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) return -1.0f;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(rayDir, q);
        if (v < 0.0f || u + v > 1.0f) return -1.0f;

        float t = f * glm::dot(edge2, q);
        if (t > EPSILON) return t;
        else return -1.0f;
    }

    void Editor::ExtractObjectByEntity(entt::entity entity, std::pair<bool, vex::GameObject*>& selectedObject){
        auto* obj = getSceneManager()->GetGameObjectByEntity(getSceneManager()->getLastSceneName(), entity);
        if(obj){
            if (obj->GetComponent<TransformComponent>().getParent() != entt::null) {
                ExtractObjectByEntity(obj->GetComponent<TransformComponent>().getParent(), selectedObject);
            }else{
                selectedObject.first = false;
                selectedObject.second = obj;
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
            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.3f, nullptr, &dock_main_id);
            ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.45f, nullptr, &dock_main_id);
            ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
            ImGuiID dock_right_top_id, dock_right_bottom_id;
            ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.5f, &dock_right_bottom_id, &dock_right_top_id);

            ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
            ImGui::DockBuilderDockWindow("Game Objects", dock_left_id);
            ImGui::DockBuilderDockWindow("Assets", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Scene", dock_right_top_id);
            ImGui::DockBuilderDockWindow("Properties", dock_right_bottom_id);
            ImGui::DockBuilderDockWindow("World Settings", dock_right_bottom_id);

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
        ImVec2 viewportPos = ImGui::GetWindowPos();
        ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

        outNewResolution = { (uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y };

        if (data.imguiTextureID) {
            ImGui::Image((ImTextureID)data.imguiTextureID, viewportPanelSize);
            bool isViewportHovered = ImGui::IsItemHovered();

            if(m_editorProperties.showFPS) {
                char fpsText[32];
                sprintf(fpsText, "FPS: %d", m_fps);

                ImU32 fpsColor;
                if (m_fps >= 60) {
                    fpsColor = IM_COL32(0, 255, 0, 255);
                } else if (m_fps >= 30) {
                    fpsColor = IM_COL32(255, 255, 0, 255);
                } else {
                    fpsColor = IM_COL32(255, 0, 0, 255);
                }

                ImVec2 textPos = ImVec2(cursorScreenPos.x + 10.0f, cursorScreenPos.y + 50.0f);
                auto* drawList = ImGui::GetWindowDrawList();
                drawList->AddText(ImVec2(textPos.x + 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 255), fpsText);
                drawList->AddText(textPos, fpsColor, fpsText);
            }

            drawGizmo(glm::vec2(cursorScreenPos.x, cursorScreenPos.y),
                                  glm::vec2(viewportPanelSize.x, viewportPanelSize.y));

            if (isViewportHovered && !m_isHoveringGizmoUI && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
                ImVec2 mousePos = ImGui::GetMousePos();
                float mouseX = mousePos.x - viewportPos.x;
                float mouseY = mousePos.y - viewportPos.y;

                float ndcX = (mouseX / viewportPanelSize.x) * 2.0f - 1.0f;
                float ndcY = (mouseY / viewportPanelSize.y) * 2.0f - 1.0f;

                auto& camTC = m_registry.get<TransformComponent>(m_camera->GetEntity());
                auto& camComp = m_registry.get<CameraComponent>(m_camera->GetEntity());

                glm::mat4 view = glm::lookAt(camTC.getWorldPosition(), camTC.getWorldPosition() + camTC.getForwardVector(), camTC.getUpVector());
                glm::mat4 proj = glm::perspective(glm::radians(camComp.fov), viewportPanelSize.x / viewportPanelSize.y, camComp.nearPlane, camComp.farPlane);
                proj[1][1] *= -1; // Vulkan Y-Flip

                glm::mat4 invProjView = glm::inverse(proj * view);

                glm::vec4 rayStartClip = glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
                glm::vec4 rayStartWorld = invProjView * rayStartClip;
                rayStartWorld /= rayStartWorld.w;

                glm::vec4 rayEndClip = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
                glm::vec4 rayEndWorld = invProjView * rayEndClip;
                rayEndWorld /= rayEndWorld.w;

                glm::vec3 rayDir = glm::normalize(glm::vec3(rayEndWorld) - glm::vec3(rayStartWorld));
                glm::vec3 rayOrigin = glm::vec3(rayStartWorld);

                float closestDist = std::numeric_limits<float>::max();
                entt::entity hitEntity = entt::null;

                auto viewGroup = m_registry.view<TransformComponent, MeshComponent>();

                for (auto entity : viewGroup) {
                    auto& mesh = viewGroup.get<MeshComponent>(entity);
                    auto& transform = viewGroup.get<TransformComponent>(entity);

                    if (raySphereIntersect(rayOrigin, rayDir, mesh.worldCenter, mesh.worldRadius) < 0.0f) {
                        continue;
                    }

                    glm::mat4 modelMat = transform.matrix();
                    glm::mat4 invModel = glm::inverse(modelMat);

                    glm::vec4 localOrigin4 = invModel * glm::vec4(rayOrigin, 1.0f);
                    glm::vec3 localOrigin = glm::vec3(localOrigin4) / localOrigin4.w;
                    glm::vec3 localDir = glm::vec3(invModel * glm::vec4(rayDir, 0.0f));
                    localDir = glm::normalize(localDir);

                    float localClosest = std::numeric_limits<float>::max();
                    bool hitMesh = false;

                    for (const auto& submesh : mesh.meshData.submeshes) {
                        for (size_t i = 0; i < submesh.indices.size(); i += 3) {
                            const auto& v0 = submesh.vertices[submesh.indices[i]].position;
                            const auto& v1 = submesh.vertices[submesh.indices[i+1]].position;
                            const auto& v2 = submesh.vertices[submesh.indices[i+2]].position;

                            float t = rayTriangleIntersect(localOrigin, localDir, v0, v1, v2);

                            if (t > 0.0f && t < localClosest) {
                                localClosest = t;
                                hitMesh = true;
                            }
                        }
                    }

                    if (hitMesh) {
                        glm::vec3 hitPointLocal = localOrigin + (localDir * localClosest);
                        glm::vec3 hitPointWorld = glm::vec3(modelMat * glm::vec4(hitPointLocal, 1.0f));

                        float dist = glm::distance(rayOrigin, hitPointWorld);

                        if (dist < closestDist) {
                            closestDist = dist;
                            hitEntity = entity;
                        }
                    }
                }
                if (hitEntity != entt::null) {
                    //m_selectedObject.first = false;

                    ExtractObjectByEntity(hitEntity, m_selectedObject);

                    log("Selected Entity ID: %d", (uint32_t)hitEntity);
                } else {
                    m_selectedObject.first = false;
                    m_selectedObject.second = nullptr;
                }
            }else{
                //log("Item hovered: %d\nMouse Clicked: %d\nImGuizmo Over: %d", ImGui::IsItemHovered(), ImGui::IsMouseClicked(ImGuiMouseButton_Left), ImGuizmo::IsOver());
            }
        }else {
            ImGui::Text("Viewport Texture Could not be retrieved.");
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Game Objects", nullptr, childFlags);
                std::vector<std::string> objectTypes = GameObjectFactory::getInstance().GetRegisteredObjectTypes();

                ImGui::Text("Available Objects:");
                ImGui::Separator();

                for (const auto& type : objectTypes) {
                    if (ImGui::Button(type.c_str(), ImVec2(-1, 0))) {
                        std::string currentScene = getSceneManager()->getLastSceneName();
                        if (!currentScene.empty()) {

                            std::string newName = "New " + type;
                            GameObject* newObj = GameObjectFactory::getInstance().create(type, *this, newName);

                            if(m_selectedObject.second){
                                newObj->AddComponent(TransformComponent{});
                                newObj->ParentTo(m_selectedObject.second->GetEntity());
                            }

                            m_frame = 0;
                            m_refresh = true;
                            newObj->BeginPlay();

                            if (newObj) {
                                getSceneManager()->GetScene(currentScene)->AddEditorGameObject(newObj);
                            }
                        } else {
                            log("Error: No scene loaded. Cannot create object.");
                        }
                    }
                }
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

        ImGui::Begin("World Settings", nullptr, childFlags);
            DrawWorldSettings(*this);
        ImGui::End();

        if (!m_pendingSceneToLoad.empty() || m_pendingSceneToLoad != "") {

            if (ImGui::Begin("Opening scene...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Loading scene: %s", m_pendingSceneToLoad.c_str());
            }
            ImGui::End();
        }
    }

    void Editor::OnHotReload() {
        log("Hot Reload detected: Deselecting objects and reloading scene...");

        m_selectedObject.first = false;
        m_selectedObject.second = nullptr;

        if (getSceneManager() && !getSceneManager()->getLastSceneName().empty()) {
            requestSceneReload(getSceneManager()->getLastSceneName());
        }
    }
}
