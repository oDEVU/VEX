/**
 *  @file   uniforms.hpp
 *  @brief  This file defines uniforms and pushconstant for rendering meshes.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <glm/glm.hpp>
#include "limits.hpp"

namespace vex {
    /// @brief Scene uniform buffer object. Holds data that is bind once per scene.
    struct SceneUBO {
        /// @brief View matrix
        alignas(16) glm::mat4 view;
        /// @brief Projection matrix
        alignas(16) glm::mat4 proj;

        alignas(4) float snapResolution;
        alignas(4) float jitterIntensity;
        /// @brief contains multiple PS1Effects thanks to bitwise operations.
        alignas(4) int enablePS1Effects;

        alignas(4) float time;
        alignas(8) glm::vec2 renderResolution;
        alignas(8) glm::vec2 windowResolution;
        alignas(4) float upscaleRatio;

        alignas(16) glm::vec4 ambientLight;
        alignas(4) float ambientLightStrength;
        alignas(16) glm::vec4 sunLight;
        alignas(16) glm::vec4 sunDirection;
    };

    /// @brief Light struct for shader
    struct Light {
        glm::vec4 position = glm::vec4(0.0f); // w = radius
        glm::vec4 color = glm::vec4(0.0f);    // w = intensity
    };

    struct SceneLightsUBO {
        alignas(4) uint32_t lightCount;
        alignas(16) Light lights[MAX_DYNAMIC_LIGHTS]; // Light is already glm::vec4 position + glm::vec4 color
    };

    /// @brief Push constants, holds model information.
    struct PushConstants {
        alignas(16) glm::vec4 color;
        alignas(16) glm::mat4 model;
    };

    /// @brief PS1Effects namespace for easier setting of them in push constant.
    /// @see VulkanMesh.cpp
    namespace PS1Effects {
        constexpr int VERTEX_SNAPPING    = 0x1;
        constexpr int AFFINE_WARPING     = 0x2;
        constexpr int SCREEN_QUANTIZATION = 0x4;
        constexpr int VERTEX_JITTER      = 0x8;
        constexpr int NTSC_ARTIFACTS     = 0x10;
        constexpr int GOURAUD_SHADING    = 0x20;
        constexpr int TEXTURE_QUANTIZATION = 0x40;
        constexpr int SCREEN_DITHER = 0x80;
    }
}
