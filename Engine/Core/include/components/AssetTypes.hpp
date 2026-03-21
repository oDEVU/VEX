#pragma once
#include <string>
#include <vector>
#include <algorithm>

#if DEBUG
#include <imgui.h>
#endif

namespace vex {

    namespace AssetExtensions {
        inline const std::vector<std::string> Mesh    = {
            ".obj", ".fbx", ".gltf", ".glb", ".dae", ".3ds", ".blend", ".ase", ".ply", ".stl"
        };

        inline const std::vector<std::string> Texture = {
            ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".hdr", ".pic", ".ppm", ".pgm"
        };

        inline const std::vector<std::string> Audio   = {
            ".wav"
        };

        /// @brief Checks if the given extension is valid for the specified asset type.
        inline bool IsValid(const std::string& ext, const std::vector<std::string>& list) {
            return std::find(list.begin(), list.end(), ext) != list.end();
        }
    }

    /// @brief Base wrapper for generic assets. Currently doesnt add any functionality. Its used mainly for custom functionality in editor
    struct asset_path : public std::string {
        using std::string::string;
        asset_path() = default;
        asset_path(const std::string& s) : std::string(s) {}
        asset_path(const char* s) : std::string(s) {}
    };

    /// @brief Specific wrapper for Meshes
    struct mesh_asset_path : public asset_path {
        using asset_path::asset_path;
        mesh_asset_path(const std::string& s) : asset_path(s) {}
    };

    /// @brief Specific wrapper for Textures
    struct texture_asset_path : public asset_path {
        using asset_path::asset_path;
        texture_asset_path(const std::string& s) : asset_path(s) {}
    };

    /// @brief Specific wrapper for Audio
    struct audio_asset_path : public asset_path {
        using asset_path::asset_path;
        audio_asset_path(const std::string& s) : asset_path(s) {}
    };
}
