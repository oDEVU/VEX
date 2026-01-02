/**
 *  @file   frustum.hpp
 *  @brief  This file defines struct holding data needed for camera frustum culling.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <glm/glm.hpp>
#include <array>

namespace vex {

/// @brief Represents a plane in 3D space.
struct Plane {
    glm::vec3 normal;
    float distance;

    void normalize() {
        float length = glm::length(normal);
        normal /= length;
        distance /= length;
    }

    /// @brief Returns signed distance from plane to point
    float getSignedDistance(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

/// @brief Represents a frustum in 3D space.
struct Frustum {
    std::array<Plane, 6> planes;

    /// @brief Extract planes from View-Projection Matrix
    /// @param const glm::mat4& viewProj View-Projection Matrix
    void update(const glm::mat4& viewProj) {
        glm::mat4 m = glm::transpose(viewProj);

        // Left
        planes[0].normal = glm::vec3(m[3] + m[0]);
        planes[0].distance = m[3].w + m[0].w;
        // Right
        planes[1].normal = glm::vec3(m[3] - m[0]);
        planes[1].distance = m[3].w - m[0].w;
        // Bottom
        planes[2].normal = glm::vec3(m[3] + m[1]);
        planes[2].distance = m[3].w + m[1].w;
        // Top
        planes[3].normal = glm::vec3(m[3] - m[1]);
        planes[3].distance = m[3].w - m[1].w;
        // Near
        planes[4].normal = glm::vec3(m[3] + m[2]);
        planes[4].distance = m[3].w + m[2].w;
        // Far
        planes[5].normal = glm::vec3(m[3] - m[2]);
        planes[5].distance = m[3].w - m[2].w;

        for (auto& plane : planes) plane.normalize();
    }

    /// @brief Returns true if sphere is visible (or partially visible)
    bool testSphere(const glm::vec3& center, float radius) const {
        for (const auto& plane : planes) {
            if (plane.getSignedDistance(center) < -radius) {
                return false;
            }
        }
        return true;
    }
};

}
