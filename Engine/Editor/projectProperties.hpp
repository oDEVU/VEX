/**
 * @file   projectProperties.hpp
 * @brief  Structure to hold all configurable projects settings and properties.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <ImReflect.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include "components/ResolutionManager.hpp"

/// @brief Window Mode Enum
enum class WindowType { Windowed = 0, Borderless = 1, ExclusiveFullscreen = 2 };

/// @brief Structure to hold all configurable project settings and properties.
struct ProjectProperties {
    std::string project_name = "NewVexProject";
    std::string main_scene = "Assets/scenes/main.json";
    std::string icon_path = "";
    std::string version = "1";

    WindowType window_type = WindowType::Windowed;
    vex::ResolutionMode resolution_mode = vex::ResolutionMode::NATIVE;
    bool vsync = true;

    auto operator<=>(const ProjectProperties&) const = default;
};

IMGUI_REFLECT(ProjectProperties, project_name, main_scene, icon_path, version, window_type, resolution_mode, vsync);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ProjectProperties, project_name, main_scene, icon_path, version, window_type, resolution_mode, vsync);
