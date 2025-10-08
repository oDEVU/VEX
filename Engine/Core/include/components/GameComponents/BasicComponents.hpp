#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <entt/entt.hpp>

#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"

namespace vex{
struct TransformComponent {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    entt::entity parent = entt::null;

    glm::mat4 matrix(const entt::registry& registry) const {
        glm::mat4 local(1.0f);
        local = glm::translate(local, position);
        local = glm::rotate(local, glm::radians(rotation.x), {1, 0, 0});
        local = glm::rotate(local, glm::radians(rotation.y), {0, 1, 0});
        local = glm::rotate(local, glm::radians(rotation.z), {0, 0, 1});
        local = glm::scale(local, scale);
        if (parent != entt::null && registry.valid(parent) && registry.all_of<TransformComponent>(parent)) {
            return registry.get<TransformComponent>(parent).matrix(registry) * local;
        }
        return local;
    }

    glm::vec3 getWorldPosition(const entt::registry& registry) const {
        return glm::vec3(matrix(registry)[3]);
    }

    glm::vec3 getWorldRotation(const entt::registry& registry) const {
        glm::vec3 worldRot = rotation;
        entt::entity current = parent;
        while (current != entt::null && registry.valid(current) && registry.all_of<TransformComponent>(current)) {
            worldRot += registry.get<TransformComponent>(current).rotation;
            current = registry.get<TransformComponent>(current).parent;
        }
        return worldRot;
    }

    glm::vec3 getWorldScale(const entt::registry& registry) const {
        glm::vec3 worldScale = scale;
        entt::entity current = parent;
        while (current != entt::null && registry.valid(current) && registry.all_of<TransformComponent>(current)) {
            worldScale *= registry.get<TransformComponent>(current).scale;
            current = registry.get<TransformComponent>(current).parent;
        }
        return worldScale;
    }

    glm::vec3 getForwardVector(const entt::registry& registry) const {
        float pitch = glm::radians(getWorldRotation(registry).x);
        float yaw = glm::radians(getWorldRotation(registry).y);

        return glm::normalize(glm::vec3(
            cos(yaw) * cos(pitch),
            sin(pitch),
            sin(yaw) * cos(pitch)
        ));
    }

    glm::vec3 getRightVector(const entt::registry& registry) {
        glm::vec3 forward = getForwardVector(registry);
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::normalize(glm::cross(forward, worldUp));
    }

    glm::vec3 getUpVector(const entt::registry& registry) {
        glm::vec3 forward = getForwardVector(registry);
        glm::vec3 right = getRightVector(registry);
        return glm::normalize(glm::cross(right, forward));
    }
};

struct MeshComponent {
    MeshData meshData;
    uint32_t id = UINT32_MAX;
    std::vector<std::string> textureNames;
};

struct NameComponent {
    std::string name;
};

struct CameraComponent {
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    uint32_t id = UINT32_MAX; // For editor use
};
}
