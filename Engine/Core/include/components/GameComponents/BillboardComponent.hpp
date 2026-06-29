/**
 *  @file   BillboardComponent.hpp
 *  @brief  Simple 2D quad rendered in 3d space, great for placeholders or visualising invisible objects
 *  @author Eryk Roszkowski
 ***********************************************/

 #pragma once
 #include "components/AssetTypes.hpp"
 #include "components/ColorTypes.hpp"

 namespace vex {
     /// @brief A component for rendering a 2D quad that always faces the camera.
     struct BillboardComponent {
         vex::texture_asset_path texturePath; ///< Path to the texture mapped onto the billboard.
         glm::vec2 size = glm::vec2(1.0f, 1.0f); ///< World-space size of the billboard quad.
         vex::rgba color = vex::rgba(1.0f, 1.0f, 1.0f, 1.0f); ///< Tint color applied to the billboard.
         bool isTransparent = false; ///< Indicates if the billboard is rendered in the transparent pass.
         bool isUnlit = false; ///< Indicates if the billboard should ignore scene lighting.
     };
 } // namespace vex

 /// @todo register that biach
