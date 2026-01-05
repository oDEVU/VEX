/**
 * @file   BasicComponents.hpp
 * @brief  Contains basic components like transform, camera, name components..
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL 1
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <vector>
#include <string>
#include <entt/entt.hpp>

#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"
#include "components/colorTypes.hpp"
#include "entt/entity/fwd.hpp"

#include "components/errorUtils.hpp"

namespace vex{

/// @brief Struct containing transform data and methods.
struct TransformComponent {
    #ifdef DEBUG
    public:
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    glm::quat m_rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    entt::entity parent = entt::null;
    bool lastTransformed = false;
    entt::registry* m_registry = nullptr;
    bool physicsAffected = false;
    #else
    private:
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::quat m_rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    entt::entity parent = entt::null;
    bool lastTransformed = false;
    entt::registry* m_registry = nullptr;
    bool physicsAffected = false;
    public:
    #endif
    /// @brief Copy Constructor. Copies all data members and maintains the reference to the same registry.
    TransformComponent(const TransformComponent& other)
        : m_registry(other.m_registry), // Initialize the reference from the other's reference
          position(other.position),
          m_rotationQuat(other.m_rotationQuat), // Copy the quaternion
          scale(other.scale),
          parent(other.parent)
    {}

    /// @brief Copy Assignment Operator. Copies all data members and maintains the reference to the same registry.
    TransformComponent& operator=(const TransformComponent& other) {
        if (this != &other) {
            position = other.position;
            m_rotationQuat = other.m_rotationQuat; // Assign the quaternion
            scale = other.scale;
            parent = other.parent;
        }
        return *this;
    }

    /// @brief Constructor 1: Registry only
    TransformComponent(entt::registry& registry)
        : m_registry(&registry) {}

    /// @brief Constructor 2: Registry, Position, (optional Parent)
    TransformComponent(entt::registry& registry, glm::vec3 pos, entt::entity p = entt::null)
        : m_registry(&registry), position(pos), parent(p) {}

    /// @brief Constructor 3: Registry, Position, Rotation (Euler degrees), (optional Parent)
    TransformComponent(entt::registry& registry, glm::vec3 pos, glm::vec3 rot, entt::entity p = entt::null)
        : m_registry(&registry), position(pos), m_rotationQuat(glm::normalize(glm::quat(glm::radians(rot)))), parent(p) {}

    /// @brief Constructor 4: Registry, Position, Rotation (Euler degrees), Scale, (optional Parent)
    TransformComponent(entt::registry& registry, glm::vec3 pos, glm::vec3 rot, glm::vec3 s, entt::entity p = entt::null)
        : m_registry(&registry), position(pos), m_rotationQuat(glm::normalize(glm::quat(glm::radians(rot)))), scale(s), parent(p) {}

    /// @brief Constructor 5: Registry, Parent only (useful for empty child objects)
    TransformComponent(entt::registry& registry, entt::entity p)
        : m_registry(&registry), parent(p) {}

    /// @brief Default constructor is deleted since registry is required
    TransformComponent() = default;

    /// @brief Check if the transform has been updated recently or if the parent has been updated recently. Used mainly in physics calculations.
    bool transformedLately(){
        bool result = false;

        if (!isReady()) {
            log("TransformComponent is not ready, registry is not valid");
            return false;
        }

        if (parent != entt::null && m_registry->valid(parent) && m_registry->all_of<TransformComponent>(parent)) {
            result = m_registry->get<TransformComponent>(parent).transformedLately();
        }
        return (lastTransformed || result);
    }

    void setRegistry(entt::registry& reg) {
        lastTransformed = true;
        m_registry = &reg;
    }

    bool isReady() const {
        return m_registry != nullptr;
    }

    #ifdef DEBUG
    void convertRot(){
        setLocalRotation(rotation);
        //rotation = glm::degrees(glm::eulerAngles(m_rotationQuat));
    }
    #endif

    /// @brief returns if objet is affected by physics.
    bool isPhysicsAffected(){
        return physicsAffected;
    }

    /// @brief Updates transform status after physics calculations. DO NOT CALL MANUALLY if you dont want to invalidate last transform changes for physics objects.
    void updatedPhysicsTransform(){
        lastTransformed = false;
        physicsAffected = true;
    }

    /// @brief Set the physics affected status.
    void setPhysicsAffected(bool value){
        physicsAffected = value;
    }

    void enableLastTransformed(){
        lastTransformed = true;
    }

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

    /// @brief Get the local rotation (as Euler angles in degrees).
    glm::vec3 getLocalRotation() const {
        return glm::degrees(glm::eulerAngles(m_rotationQuat));
    }

    /// @brief Get the local scale.
    glm::vec3 getLocalScale() const {
        return scale;
    }

    /// @brief Set the local position.
    void setLocalPosition(glm::vec3 newPosition) {
        lastTransformed = true;
        position = newPosition;
    }

    /// @brief Set the local rotation (using Euler angles in degrees).
    void setLocalRotation(glm::vec3 newRotation) {
        lastTransformed = true;
        m_rotationQuat = glm::normalize(glm::quat(glm::radians(newRotation)));
    }

    /// @brief Set the local scale.
    void setLocalScale(glm::vec3 newScale) {
        scale = newScale;
    }

        // --- WORLD QUATERNION METHODS FOR PHYSICS SYSTEMS ---

        /// @brief Method to get the world rotation as a quaternion.
        glm::quat getWorldQuaternion() const {
            if (parent != entt::null && m_registry && m_registry->valid(parent) && m_registry->all_of<TransformComponent>(parent)) {
                return m_registry->get<TransformComponent>(parent).getWorldQuaternion() * m_rotationQuat;
            }
            return m_rotationQuat;
        }

        /// @brief Method to set world rotation using a quaternion (for physics systems).
        /// @param targetWorldQuat glm::quat The desired world rotation quaternion.
        void setWorldQuaternion(glm::quat targetWorldQuat) {
            if (parent != entt::null && m_registry->valid(parent) && m_registry->all_of<TransformComponent>(parent)) {
                glm::quat parentWorldQuat = m_registry->get<TransformComponent>(parent).getWorldQuaternion();

                glm::quat parentInverse = glm::inverse(parentWorldQuat);
                m_rotationQuat = parentInverse * targetWorldQuat;
            } else {
                m_rotationQuat = targetWorldQuat;
            }
            m_rotationQuat = glm::normalize(m_rotationQuat);
        }

    // --------------------------------------------------

    /// @brief Method used by renderer to calculate the transformation matrix.
    glm::mat4 matrix() const {
        glm::mat4 local = glm::mat4(1.0f);
        local = glm::translate(local, position); // Najpierw translacja
        local *= glm::mat4_cast(m_rotationQuat); // Następnie rotacja (mnożenie macierzy)
        local = glm::scale(local, scale);        // Na koniec skala (mnożenie macierzy)

        if (parent != entt::null && m_registry && m_registry->valid(parent) && m_registry->all_of<TransformComponent>(parent)) {
            return m_registry->get<TransformComponent>(parent).matrix() * local;
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
        return glm::degrees(glm::eulerAngles(getWorldQuaternion()));
    }

    /// @brief Method to get world scale, needed when object is parented as scale parameter stores local scale.
    /// @return glm::vec3
    glm::vec3 getWorldScale() const {
        glm::vec3 worldScale = scale;
        entt::entity current = parent;
        while (current != entt::null && m_registry && m_registry->valid(current) && m_registry->all_of<TransformComponent>(current)) {
            worldScale *= m_registry->get<TransformComponent>(current).scale;
            current = m_registry->get<TransformComponent>(current).parent;
        }
        return worldScale;
    }

    /// @brief Method to set world position, needed when object is parented as position parameter stores local position.
    /// @param newPosition glm::vec3
    void setWorldPosition(glm::vec3 newPosition) {
        lastTransformed = true;
        if (parent != entt::null && m_registry && m_registry->valid(parent) && m_registry->all_of<TransformComponent>(parent)) {
            glm::mat4 parentWorldMatrix = m_registry->get<TransformComponent>(parent).matrix();
            glm::mat4 inverseParentMatrix = glm::inverse(parentWorldMatrix);
            glm::vec4 newLocalPos4 = inverseParentMatrix * glm::vec4(newPosition, 1.0f);
            position = glm::vec3(newLocalPos4);
        } else {
            position = newPosition;
        }
    }

    /// @brief Method to set world position by physics component.
    /// @param newPosition glm::vec3
    void setWorldPositionPhys(glm::vec3 newPosition) {
        if (parent != entt::null && m_registry && m_registry->valid(parent) && m_registry->all_of<TransformComponent>(parent)) {
            glm::mat4 parentWorldMatrix = m_registry->get<TransformComponent>(parent).matrix();
            glm::mat4 inverseParentMatrix = glm::inverse(parentWorldMatrix);
            glm::vec4 newLocalPos4 = inverseParentMatrix * glm::vec4(newPosition, 1.0f);
            position = glm::vec3(newLocalPos4);
        } else {
            position = newPosition;
        }
    }

    /// @brief Method to set world rotation, needed when object is parented as rotation parameter stores local rotation.
    /// @param newRotation glm::vec3
    void setWorldRotation(glm::vec3 newRotation) {
        lastTransformed = true;
        glm::quat targetWorldQuat = glm::quat(glm::radians(newRotation));
        setWorldQuaternion(targetWorldQuat);
    }

    /// @brief Method to set world scale, needed when object is parented as rotation parameter stores local rotation.
    /// @param newScale glm::vec3
    void setWorldScale(glm::vec3 newScale) {
        if (parent != entt::null && m_registry && m_registry->valid(parent) && m_registry->all_of<TransformComponent>(parent)) {
            glm::vec3 parentWorldScale = m_registry->get<TransformComponent>(parent).getWorldScale();
            scale = newScale / parentWorldScale;
        } else {
            scale = newScale;
        }
    }

    /// @brief Method to add local position.
    /// @param newPosition glm::vec3
    void addLocalPosition(glm::vec3 newPosition) {
        lastTransformed = true;
        position += newPosition;
    }

    /// @brief Method to add local rotation (Euler angles in degrees).
    /// @param newRotation glm::vec3
    void addLocalRotation(glm::vec3 newRotation) {
        lastTransformed = true;
        glm::quat deltaQuat = glm::quat(glm::radians(newRotation));
        m_rotationQuat = m_rotationQuat * deltaQuat;
        m_rotationQuat = glm::normalize(m_rotationQuat);
    }

    /// @brief Method to add local scale.
    /// @param newScale glm::vec3
    void addLocalScale(glm::vec3 newScale) {
        scale += newScale;
    }

    /// @brief Method to add to the local pitch (rotation around X-axis).
    /// @param deltaPitch float The amount to add in degrees.
    void addPitch(float deltaPitch) {
        lastTransformed = true;
        glm::quat deltaQuat = glm::angleAxis(glm::radians(deltaPitch), glm::vec3(1.0f, 0.0f, 0.0f));
        m_rotationQuat = m_rotationQuat * deltaQuat;
        m_rotationQuat = glm::normalize(m_rotationQuat);
    }

    /// @brief Method to add to the local yaw (rotation around Y-axis).
    /// @param deltaYaw float The amount to add in degrees.
    void addYaw(float deltaYaw) {
        lastTransformed = true;
        glm::quat deltaQuat = glm::angleAxis(glm::radians(deltaYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        m_rotationQuat = deltaQuat * m_rotationQuat;
        m_rotationQuat = glm::normalize(m_rotationQuat);
    }

    /// @brief Method to add to the local roll (rotation around Z-axis).
    /// @param deltaRoll float The amount to add in degrees.
    void addRoll(float deltaRoll) {
        lastTransformed = true;
        glm::quat deltaQuat = glm::angleAxis(glm::radians(deltaRoll), glm::vec3(0.0f, 0.0f, 1.0f));
        m_rotationQuat = m_rotationQuat * deltaQuat;
        m_rotationQuat = glm::normalize(m_rotationQuat);
    }


    glm::vec3 getForwardVector() const {
        return glm::normalize(-glm::vec3(matrix()[2]));
    }

    /// @brief Method to get normalized right vector.
    /// @return glm::vec3
    glm::vec3 getRightVector() const {
        return glm::normalize(glm::vec3(matrix()[0]));
    }

    /// @brief Method to get normalized up vector.
    /// @return glm::vec3
    glm::vec3 getUpVector() const {
        return glm::normalize(glm::vec3(matrix()[1]));
    }
};

#if defined(OPAQUE)
    #undef OPAQUE
#endif
#if defined(TRANSPARENT)
    #undef TRANSPARENT
#endif

/// @brief Enum class for different types of rendering. It internally switches used pipeline.
enum class RenderType {
    OPAQUE,
    TRANSPARENT,
    MASKED,
    CUSTOM
};

/// @brief Struct containing raw meshData, mesh id, texture paths and material properties. It just template and data for rendering backend to load/convert this data to its acceptible format. This is made weirdly cause its made in mind for multiple backends. IDK i hate this system but i have no better idea.
struct MeshComponent {
    // Render Data
    MeshData meshData;
    uint32_t id = UINT32_MAX;
    std::vector<std::string> textureNames;
    RenderType renderType = RenderType::OPAQUE;
    vex::rgba color = glm::vec4(1.f);

    std::map<int, std::string> textureOverrides;

    // Simple bounding data
    glm::vec3 localCenter = glm::vec3(0.0f);
    float localRadius = 1.0f;

    glm::vec3 worldCenter = glm::vec3(0.0f);
    float worldRadius = 1.0f;

    /// @brief (used internally by the engine, DO NOT CALL) returns true if the component is fresh.
    bool getIsFresh(){
        return fresh;
    }

    /// @brief (used internally by the engine, DO NOT CALL) sets the component as rendered.
    void setRendered(){
        fresh = false;
    }

    /// @brief (used internally by the engine, DO NOT CALL) forces the component to be refreshed.
    void forceRefresh(){
        fresh = true;
    }

    private:
    bool fresh = true;
};

/// @brief Struct containing light properties.
struct LightComponent {
    vex::rgb color = glm::vec3(1.f);
    float intensity = 1.f;
    float radius = 5.f;
};

/// @brief Struct containing fog properties.
struct FogComponent {
    vex::rgb color = glm::vec3(1.f);
    float density = 0.5f;
    float start = 0.0f;
    float end = 100.0f;
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
