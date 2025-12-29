/**
 * @file   editorProperties.hpp
 * @brief  Structure to hold all configurable editor settings and properties.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <ImReflect.hpp>
#include <nlohmann/json.hpp>

/// @brief Structure to hold all configurable editor settings and properties.
struct EditorProperties {
    bool showFPS = false;
    bool showCollisions = false;

    float assetBrowserThumbnailSize = 64.0f;

    float editorCameraFov = 90.0f;
    float editorCameraRenderDistance = 1000.0f;

    int frameLimit = 0;
    bool vsync = false;

    auto operator<=>(const EditorProperties&) const = default;
};

IMGUI_REFLECT(EditorProperties, showFPS, showCollisions, assetBrowserThumbnailSize, editorCameraFov, editorCameraRenderDistance, frameLimit, vsync);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EditorProperties, showFPS, showCollisions, assetBrowserThumbnailSize, editorCameraFov, editorCameraRenderDistance, frameLimit, vsync);
