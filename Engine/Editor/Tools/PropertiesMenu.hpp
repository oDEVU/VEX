#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <ImReflect.hpp>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include <string>

#include "components/GameComponents/ComponentFactory.hpp"
#include "components/GameObjects/GameObject.hpp"

inline void DrawPropertiesOfAnObject(vex::GameObject* object){
    if(!object) return;

    char buffer[256];
    strncpy(buffer, object->GetComponent<vex::NameComponent>().name.c_str(), sizeof(buffer));
    if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
        // Add name vallidation
        object->GetComponent<vex::NameComponent>().name = std::string(buffer);
    }

    ImGui::Separator();
    vex::ComponentRegistry::getInstance().drawInspectorForObject(*object);

}
