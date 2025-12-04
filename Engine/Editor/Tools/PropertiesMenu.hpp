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
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 13, 13, 255));
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

    ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float width = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((width - 150) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.00f, 0.23f, 0.01f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.47f, 0.05f, 0.05f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.03f, 0.03f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.85f, 0.85f, 1.0f));

        if (ImGui::Button("Add Component", ImVec2(150, 0))) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        ImGui::PopStyleColor(4);

        if (ImGui::BeginPopup("AddComponentPopup")) {
            auto availableComponents = vex::ComponentRegistry::getInstance().getMissingComponents(*object);

            if (availableComponents.empty()) {
                ImGui::TextDisabled("No more components available");
            } else {
                ImGui::TextDisabled("Available Components");
                ImGui::Separator();

                for (const auto& compName : availableComponents) {
                    if (ImGui::Selectable(compName.c_str())) {
                        vex::ComponentRegistry::getInstance().createComponent(*object, compName);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndPopup();
        }

}
