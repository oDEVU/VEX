#pragma once

#include <ImReflect.hpp>
#include <nlohmann/json.hpp>

struct EditorProperties {
    bool showFPS = false;

    float assetBrowserThumbnailSize = 64.0f;

    float editorCameraFov = 90.0f;
    float editorCameraRenderDistance = 1000.0f;

    auto operator<=>(const EditorProperties&) const = default;
};

IMGUI_REFLECT(EditorProperties, showFPS, assetBrowserThumbnailSize, editorCameraFov, editorCameraRenderDistance);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EditorProperties, showFPS, assetBrowserThumbnailSize, editorCameraFov, editorCameraRenderDistance);
