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
#include "entt/entity/fwd.hpp"

namespace vex{

/// @brief Struct containing transform data and methods.
/// @todo Implement methods for setting position, rotation, and scale.
struct TransformComponent {
    private:
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    entt::entity parent = entt::null;
    entt::registry& m_registry;
    public:
    /// @brief Copy Constructor. Copies all data members and maintains the reference to the same registry.
    TransformComponent(const TransformComponent& other)
        : m_registry(other.m_registry), // Initialize the reference from the other's reference
          position(other.position),
          rotation(other.rotation),
          scale(other.scale),
          parent(other.parent)
    {}

    /// @brief Copy Assignment Operator. Copies all data members and maintains the reference to the same registry.
    TransformComponent& operator=(const TransformComponent& other) {
        if (this != &other) {
            // registry_ref cannot be reassigned. Assuming destination component belongs to the same registry.
            // registry_ref = other.registry_ref; // FORBIDDEN: Cannot rebind a reference
            position = other.position;
            rotation = other.rotation;
            scale = other.scale;
            parent = other.parent;
        }
        return *this;
    }

    /// @brief Constructor 1: Registry only
    TransformComponent(entt::registry& registry)
        : m_registry(registry) {}

    /// @brief Constructor 2: Registry, Position, (optional Parent)
    TransformComponent(entt::registry& registry, glm::vec3 pos, entt::entity p = entt::null)
        : m_registry(registry), position(pos), parent(p) {}

    /// @brief Constructor 3: Registry, Position, Rotation, (optional Parent)
    TransformComponent(entt::registry& registry, glm::vec3 pos, glm::vec3 rot, entt::entity p = entt::null)
        : m_registry(registry), position(pos), rotation(rot), parent(p) {}

    /// @brief Constructor 4: Registry, Position, Rotation, Scale, (optional Parent)
    TransformComponent(entt::registry& registry, glm::vec3 pos, glm::vec3 rot, glm::vec3 s, entt::entity p = entt::null)
        : m_registry(registry), position(pos), rotation(rot), scale(s), parent(p) {}

    /// @brief Constructor 5: Registry, Parent only (useful for empty child objects)
    TransformComponent(entt::registry& registry, entt::entity p)
        : m_registry(registry), parent(p) {}

    /// @brief Default constructor is deleted since registry is required
    TransformComponent() = delete;

    /// @brief Set the parent entity.
    void setParent(entt::entity newParent) {
        parent = newParent;
    }

    /// @brief Get the parent entity.
    entt::entity getParent() const {
        return parent;
    }

    /// @brief Get the local position.
    glm::vec3 getLocalPosition() const {
        return position;
    }

    /// @brief Get the local rotation.
    glm::vec3 getLocalRotation() const {
        return rotation;
    }

    /// @brief Get the local scale.
    glm::vec3 getLocalScale() const {
        return scale;
    }

    /// @brief Set the local position.
    void setLocalPosition(glm::vec3 newPosition) {
        position = newPosition;
    }

    /// @brief Set the local rotation.
    void setLocalRotation(glm::vec3 newRotation) {
        rotation = newRotation;
    }

    /// @brief Set the local scale.
    void setLocalScale(glm::vec3 newScale) {
        scale = newScale;
    }

    /// @brief Method used by renderer to calculate the transformation matrix.
    glm::mat4 matrix() const {
        glm::mat4 local(1.0f);
        local = glm::translate(local, position);
        local = glm::rotate(local, glm::radians(rotation.x), {1, 0, 0});
        local = glm::rotate(local, glm::radians(rotation.y), {0, 1, 0});
        local = glm::rotate(local, glm::radians(rotation.z), {0, 0, 1});
        local = glm::scale(local, scale);
        if (parent != entt::null && m_registry.valid(parent) && m_registry.all_of<TransformComponent>(parent)) {
            return m_registry.get<TransformComponent>(parent).matrix() * local;
        }
        return local;
    }

    /// @brief Method to get world position, needed when object is parented as position parameter stores local position.
    /// @return glm::vec3
    glm::vec3 getWorldPosition() const {
        return glm::vec3(matrix()[3]);
    }

    /// @brief Method to get world rotation, needed when object is parented as rotation parameter stores local rotation.
    /// @return glm::vec3
    glm::vec3 getWorldRotation() const {
        glm::vec3 worldRot = rotation;
        entt::entity current = parent;
        while (current != entt::null && m_registry.valid(current) && m_registry.all_of<TransformComponent>(current)) {
            worldRot += m_registry.get<TransformComponent>(current).rotation;
            current = m_registry.get<TransformComponent>(current).parent;
        }
        return worldRot;
    }

    /// @brief Method to get world scale, needed when object is parented as scale parameter stores local scale.
    /// @return glm::vec3
    glm::vec3 getWorldScale() const {
        glm::vec3 worldScale = scale;
        entt::entity current = parent;
        while (current != entt::null && m_registry.valid(current) && m_registry.all_of<TransformComponent>(current)) {
            worldScale *= m_registry.get<TransformComponent>(current).scale;
            current = m_registry.get<TransformComponent>(current).parent;
        }
        return worldScale;
    }

    /// @brief Method to set world position, needed when object is parented as position parameter stores local position.
    /// @param newPosition glm::vec3
    void setWorldPosition(glm::vec3 newPosition) {
        glm::vec3 worldOffset = getWorldPosition() - getLocalPosition();
        position = newPosition + worldOffset;
    }

    /// @brief Method to set world rotation, needed when object is parented as rotation parameter stores local rotation.
    /// @param newRotation glm::vec3
    void setWorldRotation(glm::vec3 newRotation) {
        glm::vec3 worldOffset = getWorldRotation() - getLocalRotation();
        rotation = newRotation + worldOffset;
    }

    /// @brief Method to set world scale, needed when object is parented as scale parameter stores local scale.
    /// @param newScale glm::vec3
    void setWorldScale(glm::vec3 newScale) {
        glm::vec3 worldOffset = getWorldScale() - getLocalScale();
        scale = newScale + worldOffset;
    }

    /// @brief Method to add local position, needed when object is parented as position parameter stores local position.
    /// @param newPosition glm::vec3
    void addLocalPosition(glm::vec3 newPosition) {
        //glm::vec3 worldOffset = getWorldPosition(registry) - getLocalPosition(registry);
        position += newPosition;
    }

    /// @brief Method to add local rotation, needed when object is parented as rotation parameter stores local rotation.
    /// @param newRotation glm::vec3
    void addLocalRotation(glm::vec3 newRotation) {
        //glm::vec3 worldOffset = getWorldRotation(registry) - getLocalRotation(registry);
        rotation += newRotation;
    }

    /// @brief Method to add local scale, needed when object is parented as scale parameter stores local scale.
    /// @param newScale glm::vec3
    void addLocalScale(glm::vec3 newScale) {
        //glm::vec3 worldOffset = getWorldScale(registry) - getLocalScale(registry);
        scale += newScale;
    }

    /// @brief Method to add to the local pitch (rotation around X-axis).
    /// @param deltaPitch float The amount to add in degrees.
    void addPitch(float deltaPitch) {
        rotation.x += deltaPitch;
    }

    /// @brief Method to add to the local yaw (rotation around Y-axis).
    /// @param deltaYaw float The amount to add in degrees.
    void addYaw(float deltaYaw) {
        rotation.y += deltaYaw;
    }

    /// @brief Method to add to the local roll (rotation around Z-axis).
    /// @param deltaRoll float The amount to add in degrees.
    void addRoll(float deltaRoll) {
        rotation.z += deltaRoll;
    }

    /// @brief Method to get normalized forward vector.
    /// @return glm::vec3
    glm::vec3 getForwardVector() const {
        float pitch = glm::radians(getWorldRotation().x);
        float yaw = glm::radians(-getWorldRotation().y);

        return glm::normalize(glm::vec3(
            cos(yaw) * cos(pitch),
            sin(pitch),
            sin(yaw) * cos(pitch)
        ));
    }

    /// @brief Method to get normalized right vector.
    /// @return glm::vec3
    glm::vec3 getRightVector() {
        glm::vec3 forward = getForwardVector();
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::normalize(glm::cross(forward, worldUp));
    }

    /// @brief Method to get normalized up vector.
    /// @return glm::vec3
    glm::vec3 getUpVector() {
        glm::vec3 forward = getForwardVector();
        glm::vec3 right = getRightVector();
        return glm::normalize(glm::cross(right, forward));
    }
};

#if defined(OPAQUE)
    #undef OPAQUE
#endif
#if defined(TRANSPARENT)
    #undef TRANSPARENT
#endif

enum class RenderType  {
    OPAQUE,
    TRANSPARENT,
    CUSTOM
};

/// @brief Struct containing raw meshData, mesh id, texture paths and material properties. It just template and data for rendering backend to load/convert this data to its acceptible format. This is made weirdly cause its made in mind for multiple backends. IDK i hate this system but i have no better idea.
/// @todo Make creating this component enough to create mesh, not requiring mesh manager to load mesh data.
struct MeshComponent {
    MeshData meshData;
    uint32_t id = UINT32_MAX;
    std::vector<std::string> textureNames;
    RenderType renderType = RenderType::OPAQUE;
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
