/**
 *  @file   UIVertex.hpp
 *  @brief  This file defines structs needed for UI rendering.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <glm/glm.hpp>

namespace vex {
    /// @brief Represents a vertex for a UI element.
    struct UIVertex {
        /// @brief x, y (pixel coordinates)
        glm::vec2 position;
        /// @brief texture coordinates
        glm::vec2 uv;
        /// @brief rgba
        glm::vec4 color;
        /// @brief texture array index (-1 for solid color)
        float texIndex;
    };
    /// @brief Push constants for UI rendering.
    struct UIPushConstants {
        glm::mat4 ortho;
    };
}
