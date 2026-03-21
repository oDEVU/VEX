/**
 * @file   projectProperties.hpp
 * @brief  Structure to hold all configurable projects settings and properties.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <ImReflect.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <sstream>

#include "components/ResolutionManager.hpp"

/// @brief Window Mode Enum
enum class WindowType { Windowed = 0, Borderless = 1, ExclusiveFullscreen = 2 };

struct ProjectVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
    auto operator<=>(const ProjectVersion&) const = default;
};

IMGUI_REFLECT(ProjectVersion, major, minor, patch);

inline void to_json(nlohmann::json& j, const ProjectVersion& v) {
    j = std::to_string(v.major) + "." + std::to_string(v.minor) + "." + std::to_string(v.patch);
}

inline void from_json(const nlohmann::json& j, ProjectVersion& v) {
    std::string s;
    if (j.is_string()) s = j.get<std::string>();
    else if (j.is_number()) s = std::to_string(j.get<int>());

    v.major = 0; v.minor = 0; v.patch = 0;
    if (s.empty()) return;

    std::stringstream ss(s);
    std::string token;
    int parts[3] = {0, 0, 0};
    int i = 0;

    while (std::getline(ss, token, '.') && i < 3) {
        try { parts[i] = std::stoi(token); }
        catch (...) { parts[i] = 0; }
        if (parts[i] < 0) parts[i] = 0;
        i++;
    }
    v.major = parts[0]; v.minor = parts[1]; v.patch = parts[2];
}

/// @brief Structure to hold all configurable project settings and properties.
struct ProjectProperties {
    std::string project_name = "NewVexProject";
    std::string main_scene = "Assets/scenes/main.json";
    std::string icon_path = "";
    ProjectVersion version;

    WindowType window_type = WindowType::Windowed;
    vex::ResolutionMode resolution_mode = vex::ResolutionMode::NATIVE;
    bool vsync = true;

    auto operator<=>(const ProjectProperties&) const = default;
};

IMGUI_REFLECT(ProjectProperties, project_name, main_scene, icon_path, version, window_type, resolution_mode, vsync);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ProjectProperties, project_name, main_scene, icon_path, version, window_type, resolution_mode, vsync);
