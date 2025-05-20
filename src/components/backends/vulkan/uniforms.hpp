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
        // Time and Matrix
        alignas(16) float time;
        alignas(16) glm::vec4 affineTransformX;
        alignas(16) glm::vec4 affineTransformY;
        alignas(16) glm::vec4 affineTransformZ;

        // Vertex snapping
        alignas(8) glm::vec2 screenSize;
        alignas(4) float snapResolution;

        // PS1 Effects
        alignas(4) float jitterIntensity; // <-- REINSTATED
        alignas(4) int enablePS1Effects;

        // Color
        alignas(16) glm::vec4 color;

        // Helper method
        void setAffineTransform(const glm::mat3& m) {
            affineTransformX = glm::vec4(m[0], 0.0f);
            affineTransformY = glm::vec4(m[1], 0.0f);
            affineTransformZ = glm::vec4(m[2], 0.0f);
        }
    };

    namespace PS1Effects {
        constexpr int VERTEX_SNAPPING  = 0x1;
        constexpr int AFFINE_WARPING   = 0x2;
        constexpr int COLOR_QUANTIZATION = 0x4;
        constexpr int VERTEX_JITTER    = 0x8;
        constexpr int NTSC_ARTIFACTS    = 0x10;
    }
}
