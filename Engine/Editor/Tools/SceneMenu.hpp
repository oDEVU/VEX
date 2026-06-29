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
    enum Type { NONE, DELETE_ACTION, DUPLICATE, RENAME_START, REPARENT };
    Type type = NONE;
    vex::GameObject* target = nullptr;
    vex::GameObject* newParent = nullptr;
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

/// @brief Reparents a GameObject to another GameObject.
/// @param vex::GameObject* child - The child GameObject to reparent.
/// @param vex::GameObject* parent - The parent GameObject to reparent to.
inline void ReparentObject(vex::GameObject* child, vex::GameObject* parent) {
    if (!child || !parent || child == parent) return;
    if (!child->HasComponent<vex::TransformComponent>() || !parent->HasComponent<vex::TransformComponent>()) return;

    auto& childTc = child->GetComponent<vex::TransformComponent>();

    vex::Entity parentCheck = parent->GetEntity();
    auto& registry = child->GetEngine().getRegistry();

    while(parentCheck != vex::NULL_ENTITY && registry.has<vex::TransformComponent>(parentCheck)) {
        if(parentCheck == child->GetEntity()) {
            return;
        }
        if(registry.has<vex::TransformComponent>(parentCheck)) {
            parentCheck = registry.get<vex::TransformComponent>(parentCheck).getParent();
        } else {
            break;
        }
    }

    glm::vec3 oldWorldPos = childTc.getWorldPosition();
    glm::quat oldWorldRot = childTc.getWorldQuaternion();
    glm::vec3 oldWorldScale = childTc.getWorldScale();

    childTc.setParent(parent->GetEntity());

    childTc.setWorldPosition(oldWorldPos);
    childTc.setWorldQuaternion(oldWorldRot);
    childTc.setWorldScale(oldWorldScale);
}

/**
 * @brief Recursively draws a single GameObject node in the ImGUI tree hierarchy.
 *
 * @param vex::GameObject* obj - The GameObject to draw.
 * @param std::pair<bool, vex::GameObject*>& selectedObject - The currently selected object pair (runtime status, object pointer).
 * @param const std::unordered_map<vex::Entity, std::vector<vex::GameObject*>>& childrenMap - Map of parent entity to its children.
 * @param const std::unordered_set<vex::Entity>& runtimeSet - Set of entities created at runtime (won't be saved).
 * @param SceneAction& outAction - Output parameter to store any pending action triggered by the user (rename, delete, duplicate).
 */
inline void DrawEntityNode(
    vex::GameObject* obj,
    std::pair<bool, vex::GameObject*>& selectedObject,
    const std::unordered_map<vex::Entity, std::vector<vex::GameObject*>>& childrenMap,
    const std::unordered_set<vex::Entity>& runtimeSet,
    SceneAction& outAction,
    const char* filter = ""
) {
    if (!obj) return;

    vex::Entity entityID = obj->GetEntity();

    bool isRuntime = runtimeSet.find(entityID) != runtimeSet.end();

    std::string name = "Unnamed Object";
    if (obj->HasComponent<vex::NameComponent>()) {
        name = obj->GetComponent<vex::NameComponent>().name;
    }
    bool matchesFilter = true;
    if (filter && filter[0] != '\0') {
        std::string lowerName = name;
        std::string lowerFilter = filter;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c){ return std::tolower(c); });
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), [](unsigned char c){ return std::tolower(c); });
        if (lowerName.find(lowerFilter) == std::string::npos) {
            matchesFilter = false;
        }
    }

    std::string label = name + "##" + std::to_string((uint32_t)entityID);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (filter && filter[0] != '\0' && matchesFilter) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool isSelected = (selectedObject.second == obj);

    if (isSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool hasChildren = childrenMap.find(entityID) != childrenMap.end();
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool shouldDraw = matchesFilter || (filter && filter[0] == '\0');
    bool isOpen = false;
    
    if (shouldDraw) {
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.00f, 0.23f, 0.01f, 0.30f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.00f, 0.23f, 0.01f, 0.40f));
        }

        if (isRuntime) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 59, 3, 255));
        }

        isOpen = ImGui::TreeNodeEx((void*)(uint64_t)entityID, flags, "%s", name.c_str());
    } else {
        isOpen = true;
    }

    if (shouldDraw && ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("SCENE_HIERARCHY_NODE", &obj, sizeof(vex::GameObject*));

        ImGui::Text("Moving %s", name.c_str());
        ImGui::EndDragDropSource();
    }

    if (shouldDraw && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_NODE")) {
            vex::GameObject* droppedObject = *(vex::GameObject**)payload->Data;

            outAction.type = SceneAction::REPARENT;
            outAction.target = droppedObject;
            outAction.newParent = obj;
        }
        ImGui::EndDragDropTarget();
    }

    if (shouldDraw) {
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
                if (obj->GetComponent<vex::TransformComponent>().getParent() != vex::NULL_ENTITY) {
                    if (ImGui::MenuItem("Unparent")) {
                        obj->GetComponent<vex::TransformComponent>().setParent(vex::NULL_ENTITY);
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
    }

    if (isOpen && hasChildren) {
        const auto& children = childrenMap.at(entityID);
        for (auto* child : children) {
            DrawEntityNode(child, selectedObject, childrenMap, runtimeSet, outAction, filter);
        }
        if (shouldDraw) {
            ImGui::TreePop();
        }
    }
}

/**
 * @brief Draws the full scene hierarchy tree in an ImGUI window, handling selection, context menus, and object actions.
 *
 * @param vex::Engine& engine - Reference to the core Engine instance to access scene and registry data.
 * @param std::pair<bool, vex::GameObject*>& selectedObject - The currently selected object pair (runtime status, object pointer).
 */
inline void DrawSceneHierarchy(vex::Engine& engine, std::pair<bool, vex::GameObject*>& selectedObject) {
    static char searchBuffer[256] = "";
    ImGui::InputTextWithHint("##SearchHierarchy", "Search...", searchBuffer, sizeof(searchBuffer));
    ImGui::Separator();

    std::string sceneName = engine.getSceneManager()->getLastSceneName();

    const auto& objects = engine.getSceneManager()->GetAllObjects(sceneName);
    const auto& runtimeObjects = engine.getSceneManager()->GetAllAddedObjects(sceneName);

    if (objects.empty() && runtimeObjects.empty()) {
        ImGui::TextDisabled("No objects in scene");
        return;
    }

    std::unordered_map<vex::Entity, std::vector<vex::GameObject*>> childrenMap;
    std::vector<vex::GameObject*> rootNodes;
    std::unordered_set<vex::Entity> runtimeSet;

    auto processObjects = [&](const auto& sourceList, bool isRuntimeList) {
        for (const auto& objPtr : sourceList) {
            if (!objPtr) continue;
            vex::GameObject* obj = objPtr.get();
            vex::Entity entity = obj->GetEntity();

            if (isRuntimeList) {
                runtimeSet.insert(entity);
            }

            bool hasParent = false;
            if (obj->HasComponent<vex::TransformComponent>()) {
                auto& tc = obj->GetComponent<vex::TransformComponent>();
                vex::Entity parentID = tc.getParent();

                if (parentID != vex::NULL_ENTITY) {
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
        DrawEntityNode(root, selectedObject, childrenMap, runtimeSet, action, searchBuffer);
    }

    static bool showRenameModal = false;
        static char renameBuffer[128] = "";
        static vex::GameObject* objectToRename = nullptr;

        if (action.type == SceneAction::RENAME_START) {
            objectToRename = action.target;
            std::string currentName = "Unnamed";
            if (objectToRename->HasComponent<vex::NameComponent>()) {
                currentName = objectToRename->GetComponent<vex::NameComponent>().name;
            }
            strncpy(renameBuffer, currentName.c_str(), sizeof(renameBuffer));
            showRenameModal = true;
            ImGui::OpenPopup("Rename Object");
        }

        if (ImGui::BeginPopupModal("Rename Object", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (objectToRename) {
                ImGui::InputText("New Name", renameBuffer, sizeof(renameBuffer));

                if (ImGui::Button("Save") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    if (objectToRename->HasComponent<vex::NameComponent>()) {
                        objectToRename->GetComponent<vex::NameComponent>().name = std::string(renameBuffer);
                    }
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

                std::function<void(vex::GameObject*, vex::Entity)> recursiveCopy =
                    [&](vex::GameObject* src, vex::Entity parentEntity) {

                    std::string newName = "Unnamed (Copy)";
                    if (src->HasComponent<vex::NameComponent>()) {
                         newName = src->GetComponent<vex::NameComponent>().name + " (Copy)";
                    }
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
                        if (parentEntity != vex::NULL_ENTITY) {
                             newObj->GetComponent<vex::TransformComponent>().setParent(parentEntity);
                        } else if (src->HasComponent<vex::TransformComponent>()) {
                             vex::Entity originalParent = src->GetComponent<vex::TransformComponent>().getParent();
                             if (originalParent != vex::NULL_ENTITY) {
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

                std::string rootNewName = "Unnamed";
                if (action.target->HasComponent<vex::NameComponent>()) {
                     rootNewName = action.target->GetComponent<vex::NameComponent>().name;
                }

                std::string originalName = rootNewName;
                if (action.target->HasComponent<vex::NameComponent>()) {
                     action.target->GetComponent<vex::NameComponent>().name = rootNewName;
                }

                recursiveCopy(action.target, vex::NULL_ENTITY);

                if (action.target->HasComponent<vex::NameComponent>()) {
                     action.target->GetComponent<vex::NameComponent>().name = originalName;
                }

                engine.refreshForObject();
            }

        if (action.type == SceneAction::DELETE_ACTION && action.target) {

            engine.WaitForGpu();

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

        if (action.type == SceneAction::REPARENT && action.target && action.newParent) {
            ReparentObject(action.target, action.newParent);
        }
}
