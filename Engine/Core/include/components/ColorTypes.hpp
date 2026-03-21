#pragma once
#include <glm/glm.hpp>

namespace vex {

    /// @brief Represents an RGB color value. Used mainly for fancy rendering in editor.
    struct rgb : public glm::vec3 {
        using glm::vec3::vec3;

        rgb(const glm::vec3& v) : glm::vec3(v) {}
        rgb() = default;
    };

    /// @brief Represents an RGBA color value. Used mainly for fancy rendering in editor.
    struct rgba : public glm::vec4 {
        using glm::vec4::vec4;
        rgba(const glm::vec4& v) : glm::vec4(v) {}
        rgba() = default;
    };
}
