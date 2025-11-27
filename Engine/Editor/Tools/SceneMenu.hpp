#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <memory>

#include "Engine.hpp"
#include "components/SceneManager.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

// Renders a list of objects. Updates the 'selected' pointer when clicked.
inline void DrawSceneHierarchy(vex::Engine& engine, vex::GameObject*& selectedObject) {
    std::string sceneName = engine.getSceneManager()->getLastSceneName();
    const auto& objects = engine.getSceneManager()->GetAllObjects(sceneName);

    if (objects.size() == 0) {
        ImGui::TextDisabled("No Scene Loaded");
        return;
    }

    for (const auto& obj : objects) {
        if (!obj) continue;

        std::string name = "Unnamed Object";
        if (obj->HasComponent<vex::NameComponent>()) {
            name = obj->GetComponent<vex::NameComponent>().name;
        }

        std::string label = name + "##" + std::to_string((uint32_t)obj->GetEntity());

        bool isSelected = (selectedObject == obj.get());

        if (ImGui::Selectable(label.c_str(), isSelected)) {
            selectedObject = obj.get();
        }
    }
}
