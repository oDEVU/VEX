/**
 *  @file   uniforms.hpp
 *  @brief  This file defines uniforms and pushconstant for rendering meshes.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <glm/glm.hpp>

namespace vex {
    /// @brief Camera uniform buffer object.
    struct CameraUBO {
        /// @brief View matrix
        alignas(16) glm::mat4 view;
        /// @brief Projection matrix
        alignas(16) glm::mat4 proj;
    };

    /// @brief Model uniform buffer object.
    struct ModelUBO {
        /// @brief Model matrix
        alignas(16) glm::mat4 model;
    };

    /// @brief Push constants, holds both environment and model information.
    struct PushConstants {
        alignas(4) float snapResolution;
        alignas(4) float jitterIntensity;
        /// @brief contains multiple PS1Effects thanks to bitwise operations.
        alignas(4) int enablePS1Effects;

        alignas(16) glm::vec4 color;

        alignas(4) float time;
        alignas(8) glm::vec2 renderResolution;
        alignas(8) glm::vec2 windowResolution;
        alignas(4) float upscaleRatio;
        alignas(4) int renderingMode;

        alignas(16) glm::vec4 ambientLight;
        alignas(4) float ambientLightStrength;
        alignas(16) glm::vec4 sunLight;
        alignas(16) glm::vec4 sunDirection;
    };

    /// @brief PS1Effects namespace for easier setting of them in push constant.
    /// @see VulkanMesh.cpp
    namespace PS1Effects {
        constexpr int VERTEX_SNAPPING    = 0x1;
        constexpr int AFFINE_WARPING     = 0x2;
        constexpr int COLOR_QUANTIZATION = 0x4;
        constexpr int VERTEX_JITTER      = 0x8;
        constexpr int NTSC_ARTIFACTS     = 0x10;
        constexpr int GOURAUD_SHADING    = 0x20;
    }
}
