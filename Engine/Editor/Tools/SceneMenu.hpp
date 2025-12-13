/**
 * @file   SceneMenu.hpp
 * @brief  Utility functions for drawing the Scene Hierarchy and related object manipulation logic.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#ifndef _WIN32
    #include <cxxabi.h>
#endif

#include "Engine.hpp"
#include "components/SceneManager.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

/// @brief Structure to define an action to be performed on a scene object outside of the hierarchy drawing loop.
struct SceneAction {
    enum Type { NONE, DELETE_ACTION, DUPLICATE, RENAME_START };
    Type type = NONE;
    vex::GameObject* target = nullptr;
};

/**
 * @brief Demangles a C++ type name into a more readable string.
 * @param const char* name - The raw type name (e.g., from typeid().name()).
 * @return std::string - The demangled class/struct name.
 */
inline std::string Demangle(const char* name) {
#ifdef _WIN32
    std::string s = name;
    const std::string prefix_class = "class ";
    const std::string prefix_struct = "struct ";

    if (s.rfind(prefix_class, 0) == 0) return s.substr(prefix_class.length());
    if (s.rfind(prefix_struct, 0) == 0) return s.substr(prefix_struct.length());
    return s;
#else
    int status = -1;
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
#endif
}

/**
 * @brief Recursively draws a single GameObject node in the ImGUI tree hierarchy.
 *
 * @param vex::GameObject* obj - The GameObject to draw.
 * @param std::pair<bool, vex::GameObject*>& selectedObject - The currently selected object pair (runtime status, object pointer).
 * @param const std::unordered_map<entt::entity, std::vector<vex::GameObject*>>& childrenMap - Map of parent entity to its children.
 * @param const std::unordered_set<entt::entity>& runtimeSet - Set of entities created at runtime (won't be saved).
 * @param SceneAction& outAction - Output parameter to store any pending action triggered by the user (rename, delete, duplicate).
 */
inline void DrawEntityNode(
    vex::GameObject* obj,
    std::pair<bool, vex::GameObject*>& selectedObject,
    const std::unordered_map<entt::entity, std::vector<vex::GameObject*>>& childrenMap,
    const std::unordered_set<entt::entity>& runtimeSet,
    SceneAction& outAction
) {
    if (!obj) return;

    entt::entity entityID = obj->GetEntity();

    bool isRuntime = runtimeSet.find(entityID) != runtimeSet.end();

    std::string name = "Unnamed Object";
    if (obj->HasComponent<vex::NameComponent>()) {
        name = obj->GetComponent<vex::NameComponent>().name;
    }
    std::string label = name + "##" + std::to_string((uint32_t)entityID);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    bool isSelected = (selectedObject.second == obj);

    if (isSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.00f, 0.23f, 0.01f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.00f, 0.23f, 0.01f, 0.40f));
    }

    if (selectedObject.second == obj) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool hasChildren = childrenMap.find(entityID) != childrenMap.end();
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (isRuntime) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 59, 3, 255));
    }

    bool isOpen = ImGui::TreeNodeEx((void*)(uint64_t)entityID, flags, "%s", name.c_str());

    if (isRuntime) {
        ImGui::PopStyleColor();
    }

    if (isSelected) {
        ImGui::PopStyleColor(2);
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedObject.second = obj;
        selectedObject.first = isRuntime;
    }

    if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename")) {
                outAction.type = SceneAction::RENAME_START;
                outAction.target = obj;
            }

            if (ImGui::MenuItem("Duplicate")) {
                outAction.type = SceneAction::DUPLICATE;
                outAction.target = obj;
            }

            ImGui::Separator();

            if (obj->HasComponent<vex::TransformComponent>()) {
                if (obj->GetComponent<vex::TransformComponent>().getParent() != entt::null) {
                    if (ImGui::MenuItem("Unparent")) {
                        obj->GetComponent<vex::TransformComponent>().setParent(entt::null);
                    }
                }
            }

            if (ImGui::MenuItem("Delete")) {
                outAction.type = SceneAction::DELETE_ACTION;
                outAction.target = obj;
            }

            ImGui::EndPopup();
        }

    if (ImGui::IsItemHovered()) {
        vex::GameObject* raw = obj;
        const char* rawName = typeid(*raw).name();
        std::string className = Demangle(rawName);

        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.00f, 0.23f, 0.01f, 1.0f), "Object Details");
        ImGui::Separator();
        ImGui::Text("C++ Class: %s", className.c_str());
        ImGui::Text("Entity ID: %u", (uint32_t)entityID);
        if (isRuntime) {
            ImGui::TextColored(ImVec4(0.47f, 0.05f, 0.05f, 1.0f), "Warning: Object created at runtime\n(Won't be saved)");
        }
        ImGui::EndTooltip();
    }

    if (isOpen && hasChildren) {
        const auto& children = childrenMap.at(entityID);
        for (auto* child : children) {
            DrawEntityNode(child, selectedObject, childrenMap, runtimeSet, outAction);
        }
        ImGui::TreePop();
    }
}

/**
 * @brief Draws the full scene hierarchy tree in an ImGUI window, handling selection, context menus, and object actions.
 *
 * @param vex::Engine& engine - Reference to the core Engine instance to access scene and registry data.
 * @param std::pair<bool, vex::GameObject*>& selectedObject - The currently selected object pair (runtime status, object pointer).
 */
inline void DrawSceneHierarchy(vex::Engine& engine, std::pair<bool, vex::GameObject*>& selectedObject) {
    std::string sceneName = engine.getSceneManager()->getLastSceneName();

    const auto& objects = engine.getSceneManager()->GetAllObjects(sceneName);
    const auto& runtimeObjects = engine.getSceneManager()->GetAllAddedObjects(sceneName);

    if (objects.empty() && runtimeObjects.empty()) {
        ImGui::TextDisabled("No objects in scene");
        return;
    }

    std::unordered_map<entt::entity, std::vector<vex::GameObject*>> childrenMap;
    std::vector<vex::GameObject*> rootNodes;
    std::unordered_set<entt::entity> runtimeSet;

    auto processObjects = [&](const auto& sourceList, bool isRuntimeList) {
        for (const auto& objPtr : sourceList) {
            if (!objPtr) continue;
            vex::GameObject* obj = objPtr.get();
            entt::entity entity = obj->GetEntity();

            if (isRuntimeList) {
                runtimeSet.insert(entity);
            }

            bool hasParent = false;
            if (obj->HasComponent<vex::TransformComponent>()) {
                auto& tc = obj->GetComponent<vex::TransformComponent>();
                entt::entity parentID = tc.getParent();

                if (parentID != entt::null && engine.getRegistry().valid(parentID)) {
                    childrenMap[parentID].push_back(obj);
                    hasParent = true;
                }
            }

            if (!hasParent) {
                rootNodes.push_back(obj);
            }
        }
    };

    processObjects(objects, false);
    processObjects(runtimeObjects, true);

    SceneAction action;

    for (auto* root : rootNodes) {
        DrawEntityNode(root, selectedObject, childrenMap, runtimeSet, action);
    }

    static bool showRenameModal = false;
        static char renameBuffer[128] = "";
        static vex::GameObject* objectToRename = nullptr;

        if (action.type == SceneAction::RENAME_START) {
            objectToRename = action.target;
            std::string currentName = objectToRename->GetComponent<vex::NameComponent>().name;
            strncpy(renameBuffer, currentName.c_str(), sizeof(renameBuffer));
            showRenameModal = true;
            ImGui::OpenPopup("Rename Object");
        }

        if (ImGui::BeginPopupModal("Rename Object", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (objectToRename) {
                ImGui::InputText("New Name", renameBuffer, sizeof(renameBuffer));

                if (ImGui::Button("Save") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    objectToRename->GetComponent<vex::NameComponent>().name = std::string(renameBuffer);
                    showRenameModal = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    showRenameModal = false;
                    ImGui::CloseCurrentPopup();
                }
            } else {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (action.type == SceneAction::DUPLICATE && action.target) {

                std::function<void(vex::GameObject*, entt::entity)> recursiveCopy =
                    [&](vex::GameObject* src, entt::entity parentEntity) {

                    std::string newName = src->GetComponent<vex::NameComponent>().name + " (Copy)";
                    vex::GameObject* newObj = vex::GameObjectFactory::getInstance().create(src->getObjectType(), engine, newName);

                    if (!newObj) return;

                    const auto& regNames = vex::ComponentRegistry::getInstance().getRegisteredNames();
                    for (const auto& compName : regNames) {
                        nlohmann::json compData = vex::ComponentRegistry::getInstance().saveComponent(*src, compName);
                        if (!compData.is_null()) {
                             vex::ComponentRegistry::getInstance().loadComponent(*newObj, compName, compData);
                        }
                    }

                    if (newObj->HasComponent<vex::NameComponent>()) {
                        newObj->GetComponent<vex::NameComponent>().name = newName;
                    }

                    if (newObj->HasComponent<vex::TransformComponent>()) {
                        if (parentEntity != entt::null) {
                             newObj->GetComponent<vex::TransformComponent>().setParent(parentEntity);
                        } else if (src->HasComponent<vex::TransformComponent>()) {
                             entt::entity originalParent = src->GetComponent<vex::TransformComponent>().getParent();
                             if (originalParent != entt::null) {
                                 newObj->GetComponent<vex::TransformComponent>().setParent(originalParent);
                             }
                        }
                    }

                    engine.getSceneManager()->GetScene(sceneName)->AddEditorGameObject(newObj);

                    if (childrenMap.find(src->GetEntity()) != childrenMap.end()) {
                        for (auto* child : childrenMap.at(src->GetEntity())) {
                            if (runtimeSet.find(child->GetEntity()) == runtimeSet.end()) {
                                recursiveCopy(child, newObj->GetEntity());
                            }
                        }
                    }
                };

                std::string rootNewName = action.target->GetComponent<vex::NameComponent>().name;

                std::string originalName = action.target->GetComponent<vex::NameComponent>().name;
                action.target->GetComponent<vex::NameComponent>().name = rootNewName;

                recursiveCopy(action.target, entt::null);

                action.target->GetComponent<vex::NameComponent>().name = originalName;

                engine.refreshForObject();
            }

        if (action.type == SceneAction::DELETE_ACTION && action.target) {

                std::function<void(vex::GameObject*)> recursiveDelete =
                    [&](vex::GameObject* targetObj) {

                    if (childrenMap.find(targetObj->GetEntity()) != childrenMap.end()) {
                        auto children = childrenMap.at(targetObj->GetEntity());
                        for (auto* child : children) {
                            recursiveDelete(child);
                        }
                    }

                    if (selectedObject.second == targetObj) {
                        selectedObject.first = false;
                        selectedObject.second = nullptr;
                    }

                    auto* scene = engine.getSceneManager()->GetScene(sceneName);
                    if (scene) {
                        scene->DestroyGameObject(targetObj);
                    }
                };

                recursiveDelete(action.target);
            }
}
