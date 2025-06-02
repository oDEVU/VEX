#pragma once
#include "transform.hpp"
#include <memory>

namespace vex {
    class Camera {
        // re make it properly when it works
    public:
        Transform transform;
        float fov = 45;
        float nearPlane = 0.1;
        float farPlane = 100.0;
        uint32_t id = UINT32_MAX;
    };
}
