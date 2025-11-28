#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <ImReflect.hpp>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include <string>

#include "components/GameComponents/ComponentFactory.hpp"
#include "components/GameObjects/GameObject.hpp"

inline void DrawPropertiesOfAnObject(vex::GameObject* object, bool temporary){
    if(!object) return;

    if(temporary){
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,225,0,255));
        ImGui::Text("[WARNING]");
        ImGui::TextWrapped("This object was created at runtime. Anything you edit here will be not be saved. To edit this object you need to find where in code it was created.");
        ImGui::PopStyleColor();
    }

    char buffer[256];
    strncpy(buffer, object->GetComponent<vex::NameComponent>().name.c_str(), sizeof(buffer));
    if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
        // Add name vallidation
        object->GetComponent<vex::NameComponent>().name = std::string(buffer);
    }

    ImGui::Separator();
    vex::ComponentRegistry::getInstance().drawInspectorForObject(*object);

}
