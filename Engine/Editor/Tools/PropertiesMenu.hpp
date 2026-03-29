/**
 * @file   PropertiesMenu.hpp
 * @brief  Utility function for drawing the component properties inspector for a selected GameObject.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <ImReflect.hpp>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include <string>
#include <algorithm>
#include <cctype>

#include "components/GameComponents/ComponentFactory.hpp"
#include "components/GameObjects/GameObject.hpp"

/**
 * @brief Draws the properties panel for a given GameObject, including its name and all components.
 *
 * @param vex::GameObject* object - The GameObject whose properties are to be displayed.
 * @param bool temporary - True if the object was created at runtime and is temporary (shows a warning).
 */
inline void DrawPropertiesOfAnObject(vex::GameObject* object, bool temporary){
    if(!object) return;

    if(temporary){
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 13, 13, 255));
        ImGui::Text("[WARNING]");
        ImGui::TextWrapped("This object was created at runtime. Anything you edit here will be not be saved. To edit this object you need to find where in code it was created.");
        ImGui::PopStyleColor();
    }

    char buffer[256] = "";
    if (object->HasComponent<vex::NameComponent>()) {
        strncpy(buffer, object->GetComponent<vex::NameComponent>().name.c_str(), sizeof(buffer));
        if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
            object->GetComponent<vex::NameComponent>().name = std::string(buffer);
        }
    } else {
        ImGui::TextDisabled("No NameComponent");
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
                static char searchBuffer[256] = "";
                ImGui::InputTextWithHint("##SearchComponent", "Search...", searchBuffer, sizeof(searchBuffer));
                ImGui::Separator();

                ImGui::TextDisabled("Available Components");
                ImGui::Separator();

                for (const auto& compName : availableComponents) {
                    if (searchBuffer[0] != '\0') {
                        std::string lowerName = compName;
                        std::string lowerSearch = searchBuffer;
                        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c){ return std::tolower(c); });
                        std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), [](unsigned char c){ return std::tolower(c); });
                        if (lowerName.find(lowerSearch) == std::string::npos) {
                            continue;
                        }
                    }

                    if (ImGui::Selectable(compName.c_str())) {
                        vex::ComponentRegistry::getInstance().createComponent(*object, compName);
                        ImGui::CloseCurrentPopup();
                        searchBuffer[0] = '\0';
                    }
                }
            }
            ImGui::EndPopup();
        }

}
