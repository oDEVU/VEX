#pragma once
#include <glm/glm.hpp>

namespace vex {
    struct CameraUBO {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct ModelUBO {
        alignas(16) glm::mat4 model;
    };

    struct PushConstants {
        alignas(4) float snapResolution;
        alignas(4) float jitterIntensity;
        alignas(4) int enablePS1Effects;

        alignas(16) glm::vec4 color;

        alignas(16) float time;
        alignas(8) glm::vec2 renderResolution;
        alignas(8) glm::vec2 windowResolution;
        alignas(16) float upscaleRatio;
        alignas(4) int renderingMode;
    };

    namespace PS1Effects {
        constexpr int VERTEX_SNAPPING  = 0x1;
        constexpr int AFFINE_WARPING   = 0x2;
        constexpr int COLOR_QUANTIZATION = 0x4;
        constexpr int VERTEX_JITTER    = 0x8;
        constexpr int NTSC_ARTIFACTS    = 0x10;
    }
}
