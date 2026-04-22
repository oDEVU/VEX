/**
 *  @file   EditorBillboardComponent.hpp
 *  @brief  Simple 2D quad rendered in 3d space exclusevely for editor.
 *  @author Eryk Roszkowski
 ***********************************************/
#pragma once
#include <components/AssetTypes.hpp>
#include <vector>

namespace vex {
    /// @brief A component for rendering a 2D quad that always faces the camera.
    struct EditorBillboardComponent {
        std::vector<vex::texture_asset_path> texturePaths; ///< Paths to the textures mapped onto the billboard.
    };
} // namespace vex
