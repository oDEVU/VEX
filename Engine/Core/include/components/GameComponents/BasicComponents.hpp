/**
 *  @file   BasicComponents.hpp
 *  @brief  Contains basic components like transform, camera, name components..
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <entt/entt.hpp>

#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"

namespace vex{

/// @brief Struct containing transform data and methods.
/// @todo Implement methods for setting position, rotation, and scale.
struct TransformComponent {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    entt::entity parent = entt::null;

    /// @brief Method used by renderer to calculate the transformation matrix.
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

    /// @brief Method to get world position, needed when object is parented as position parameter stores local position.
    /// @return glm::vec3
    glm::vec3 getWorldPosition(const entt::registry& registry) const {
        return glm::vec3(matrix(registry)[3]);
    }

    /// @brief Method to get world rotation, needed when object is parented as rotation parameter stores local rotation.
    /// @return glm::vec3
    glm::vec3 getWorldRotation(const entt::registry& registry) const {
        glm::vec3 worldRot = rotation;
        entt::entity current = parent;
        while (current != entt::null && registry.valid(current) && registry.all_of<TransformComponent>(current)) {
            worldRot += registry.get<TransformComponent>(current).rotation;
            current = registry.get<TransformComponent>(current).parent;
        }
        return worldRot;
    }

    /// @brief Method to get world scale, needed when object is parented as scale parameter stores local scale.
    /// @return glm::vec3
    glm::vec3 getWorldScale(const entt::registry& registry) const {
        glm::vec3 worldScale = scale;
        entt::entity current = parent;
        while (current != entt::null && registry.valid(current) && registry.all_of<TransformComponent>(current)) {
            worldScale *= registry.get<TransformComponent>(current).scale;
            current = registry.get<TransformComponent>(current).parent;
        }
        return worldScale;
    }

    /// @brief Method to get normalized forward vector.
    /// @return glm::vec3
    glm::vec3 getForwardVector(const entt::registry& registry) const {
        float pitch = glm::radians(getWorldRotation(registry).x);
        float yaw = glm::radians(getWorldRotation(registry).y);

        return glm::normalize(glm::vec3(
            cos(yaw) * cos(pitch),
            sin(pitch),
            sin(yaw) * cos(pitch)
        ));
    }

    /// @brief Method to get normalized right vector.
    /// @return glm::vec3
    glm::vec3 getRightVector(const entt::registry& registry) {
        glm::vec3 forward = getForwardVector(registry);
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::normalize(glm::cross(forward, worldUp));
    }

    /// @brief Method to get normalized up vector.
    /// @return glm::vec3
    glm::vec3 getUpVector(const entt::registry& registry) {
        glm::vec3 forward = getForwardVector(registry);
        glm::vec3 right = getRightVector(registry);
        return glm::normalize(glm::cross(right, forward));
    }
};

/// @brief Struct containing raw meshData, mesh id, texture paths and material properties. It just template and data for rendering backend to load/convert this data to its acceptible format. This is made weirdly cause its made in mind for multiple backends. IDK i hate this system but i have no better idea.
/// @todo Make creating this component enough to create mesh, not requiring mesh manager to load mesh data.
struct MeshComponent {
    MeshData meshData;
    uint32_t id = UINT32_MAX;
    std::vector<std::string> textureNames;
    bool isTransparent = false;
};

/// @brief Struct that simply contains name of the entity. It is used to identify entity and needs to be unique.
struct NameComponent {
    std::string name;
};

/// @brief Struct that contains camera properties. Used by build in CameraObject, but needed for any custom one for engine to recognise object as valid CameraObject.
struct CameraComponent {
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    uint32_t id = UINT32_MAX; // For editor use
};
}
