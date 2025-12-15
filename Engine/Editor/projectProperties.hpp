/**
 * @file   projectProperties.hpp
 * @brief  Structure to hold all configurable projects settings and properties.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <ImReflect.hpp>
#include <nlohmann/json.hpp>
#include <string>

/// @brief Structure to hold all configurable project settings and properties.
struct ProjectProperties {
    std::string project_name = "New Project";
    std::string main_scene = "Assets/scenes/main.json";
    std::string icon_path = "";
    std::string version = "1";

    auto operator<=>(const ProjectProperties&) const = default;
};

IMGUI_REFLECT(ProjectProperties, project_name, main_scene, icon_path, version);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ProjectProperties, project_name, main_scene, icon_path, version);
