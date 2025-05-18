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
        alignas(16) glm::vec3 color; // Optional material color
    };
}
