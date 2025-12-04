#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <cxxabi.h>

#include "Engine.hpp"
#include "components/SceneManager.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

inline std::string Demangle(const char* name) {
    int status = -1;
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
}

inline void DrawEntityNode(
    vex::GameObject* obj,
    std::pair<bool, vex::GameObject*>& selectedObject,
    const std::unordered_map<entt::entity, std::vector<vex::GameObject*>>& childrenMap,
    const std::unordered_set<entt::entity>& runtimeSet
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

    if (selectedObject.second == obj) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool hasChildren = childrenMap.find(entityID) != childrenMap.end();
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (isRuntime) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 225, 0, 255));
    }

    bool isOpen = ImGui::TreeNodeEx((void*)(uint64_t)entityID, flags, "%s", name.c_str());

    if (isRuntime) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedObject.second = obj;
        selectedObject.first = isRuntime;
    }

    if (ImGui::IsItemHovered()) {
        vex::GameObject* raw = obj;
        const char* rawName = typeid(*raw).name();
        std::string className = Demangle(rawName);

        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(0.2f, 0.4f, 1.0f, 1.0f), "Object Details");
        ImGui::Separator();
        ImGui::Text("C++ Class: %s", className.c_str());
        ImGui::Text("Entity ID: %u", (uint32_t)entityID);
        if (isRuntime) {
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.0f, 1.0f), "Warning: Object created at runtime\n(Won't be saved)");
        }
        ImGui::EndTooltip();
    }

    if (isOpen && hasChildren) {
        const auto& children = childrenMap.at(entityID);
        for (auto* child : children) {
            DrawEntityNode(child, selectedObject, childrenMap, runtimeSet);
        }
        ImGui::TreePop();
    }
}

inline void DrawSceneHierarchy(vex::Engine& engine, std::pair<bool, vex::GameObject*>& selectedObject) {
    std::string sceneName = engine.getSceneManager()->getLastSceneName();

    const auto& objects = engine.getSceneManager()->GetAllObjects(sceneName);
    const auto& runtimeObjects = engine.getSceneManager()->GetAllAddedObjects(sceneName);

    if (objects.empty() && runtimeObjects.empty()) {
        ImGui::TextDisabled("No Scene Loaded");
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

    for (auto* root : rootNodes) {
        DrawEntityNode(root, selectedObject, childrenMap, runtimeSet);
    }
}
