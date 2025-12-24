/**
 *  @file   PhysicsSystem.hpp
 *  @brief  This file defines PhysicsSystem class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL 1
#endif
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <mutex>
#include <optional>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include <components/GameComponents/BasicComponents.hpp>
#include <components/GameComponents/CharacterComponent.hpp>

#include "components/JoltSafe.hpp"

namespace vex {

    struct BodyIDHasher {
        std::size_t operator()(const JPH::BodyID& id) const {
            return std::hash<uint32_t>{}(id.GetIndexAndSequenceNumber());
        }
    };

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
    public:
        // @brief Returns the number of broad phase layers.
        unsigned int GetNumBroadPhaseLayers() const override { return 1; }
        // @brief Returns the broad phase layer for a given object layer.
        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override { return JPH::BroadPhaseLayer(0); }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        // @brief Returns the name of a broad phase layer for profiling purposes.
        const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override { return "Default"; }
#endif
    };

    class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        // @brief Determines if an object layer should collide with a broad phase layer.
        bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override { return true; }
    };

    class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
    public:
        // @brief Determines if two object layers should collide.
        bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override { return true; }
    };

    class MyActivationListener : public JPH::BodyActivationListener {
    public:
        // @brief Called when a body is activated.
        void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override {}
        // @brief Called **change** Called when a body is deactivated.
        void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override {}
    };

    /// @brief Enumeration of available shape types for physics components.
    enum class ShapeType {
        BOX, ROUNDED_BOX, SPHERE, CAPSULE, CYLINDER, CONVEX_HULL, MESH
    };

    /// @brief Enumeration of available body types for physics components.
    enum class BodyType {
        STATIC, DYNAMIC, KINEMATIC, SENSOR
    };

    /// @brief Structure representing a collision hit.
    struct CollisionHit {
        glm::vec3 position;
        glm::vec3 normal;
        float impulse;
    };

    /// @brief Structure representing a raycast hit.
    struct RaycastHit {
        JPH::BodyID bodyId;
        float distance;
        glm::vec3 position;
        glm::vec3 normal;
    };

    /// @brief Structure representing a physics component.
    struct PhysicsComponent {
        ShapeType shape = ShapeType::BOX;
        BodyType bodyType = BodyType::STATIC;
        float mass = 1.0f;
        float friction = 0.5f;
        float bounce = 0.1f;
        uint8_t objectLayer = 0;
        JPH::BodyID bodyId = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
        float linearDamping = 0.05f;
        float angularDamping = 0.05f;
        bool isSensor = false;
        bool allowSleeping = true;

        glm::vec3 boxHalfExtents = {0.5f, 0.5f, 0.5f};
        float roundedRadius = 0.05f;
        float sphereRadius = 0.5f;
        float capsuleRadius = 0.5f;
        float capsuleHeight = 1.0f;
        float cylinderRadius = 0.5f;
        float cylinderHeight = 1.0f;
        std::vector<JPH::Vec3> convexPoints;
        std::vector<glm::vec3> meshVertices;
        std::vector<uint32_t> meshIndices;

        // Collision callbacks
        std::function<void(entt::entity self, entt::entity other, const CollisionHit& hit)> onCollisionEnter;
        std::function<void(entt::entity self, entt::entity other, const CollisionHit& hit)> onCollisionStay;
        std::function<void(entt::entity self, entt::entity other)> onCollisionExit;

        #if DEBUG
        bool updated = false;
        #endif

        /// --- Constructors ---
        PhysicsComponent() = default;

        explicit PhysicsComponent(ShapeType s) : shape(s) {}

        // @brief Creates a box-shaped physics component.
        static PhysicsComponent Box(glm::vec3 halfExtents = {0.5f, 0.5f, 0.5f}, BodyType bodyType = BodyType::STATIC, float mass = 1.0f, float friction = 0.5f, float bounce = 0.1f) {
            PhysicsComponent pc;
            pc.shape = ShapeType::BOX;
            pc.boxHalfExtents = halfExtents;
            pc.mass = mass;
            pc.bodyType = bodyType;
            pc.friction = friction;
            pc.bounce = bounce;
            return pc;
        }

        // @brief Creates a rounded box
            static PhysicsComponent RoundedBox(glm::vec3 halfExtents, float radius = 0.05f, BodyType bodyType = BodyType::DYNAMIC, float mass = 1.0f, float friction = 0.5f, float bounce = 0.1f) {
                PhysicsComponent pc;
                pc.shape = ShapeType::ROUNDED_BOX;
                pc.boxHalfExtents = halfExtents;
                pc.roundedRadius = radius;
                pc.mass = mass;
                pc.bodyType = bodyType;
                pc.friction = friction;
                pc.bounce = bounce;
                return pc;
            }

        // @brief Creates a sphere-shaped physics component.
        static PhysicsComponent Sphere(float radius = 0.5f, BodyType bodyType = BodyType::STATIC, float mass = 1.0f, float friction = 0.5f, float bounce = 0.1f) {
            PhysicsComponent pc;
            pc.shape = ShapeType::SPHERE;
            pc.sphereRadius = radius;
            pc.mass = mass;
            pc.bodyType = bodyType;
            pc.friction = friction;
            pc.bounce = bounce;
            return pc;
        }

        // @brief Creates a capsule-shaped physics component.
        static PhysicsComponent Capsule(float radius = 0.5f, float height = 1.0f, BodyType bodyType = BodyType::STATIC, float mass = 1.0f, float friction = 0.5f, float bounce = 0.1f) {
            PhysicsComponent pc;
            pc.shape = ShapeType::CAPSULE;
            pc.capsuleRadius = radius;
            pc.capsuleHeight = height;
            pc.mass = mass;
            pc.bodyType = bodyType;
            pc.friction = friction;
            pc.bounce = bounce;
            return pc;
        }

        // @brief Creates a cylinder-shaped physics component.
        static PhysicsComponent Cylinder(float radius = 0.5f, float height = 1.0f, BodyType bodyType = BodyType::STATIC, float mass = 1.0f, float friction = 0.5f, float bounce = 0.1f) {
            PhysicsComponent pc;
            pc.shape = ShapeType::CYLINDER;
            pc.cylinderRadius = radius;
            pc.cylinderHeight = height;
            pc.mass = mass;
            pc.bodyType = bodyType;
            pc.friction = friction;
            pc.bounce = bounce;
            return pc;
        }

        // @brief Creates a convex hull-shaped physics component.
        static PhysicsComponent ConvexHull(std::vector<JPH::Vec3> points, BodyType bodyType = BodyType::STATIC, float mass = 1.0f, float friction = 0.5f, float bounce = 0.1f) {
            PhysicsComponent pc;
            pc.shape = ShapeType::CONVEX_HULL;
            pc.convexPoints = points;
            pc.mass = mass;
            pc.bodyType = bodyType;
            pc.friction = friction;
            pc.bounce = bounce;
            return pc;
        }

        // @brief Creates a mesh-shaped physics component.
        static PhysicsComponent Mesh(MeshComponent& mesh, BodyType bodyType = BodyType::STATIC, float mass = 1.0f, float friction = 0.5f, float bounce = 0.1f) {
            PhysicsComponent pc;
            pc.shape = ShapeType::MESH;
            pc.bodyType = bodyType;
            pc.mass = mass;
            pc.friction = friction;
            pc.bounce = bounce;
            if (!mesh.meshData.submeshes.empty()) {
                pc.meshVertices.clear();
                pc.meshIndices.clear();

                size_t vertexOffset = 0;

                for (const auto& sm : mesh.meshData.submeshes) {
                    for (const auto& v : sm.vertices) {
                        pc.meshVertices.push_back(v.position);
                    }
                    for (auto idx : sm.indices) {
                        pc.meshIndices.push_back(static_cast<uint32_t>(idx + vertexOffset));
                    }
                    vertexOffset += sm.vertices.size();
                }
            }
            return pc;
        }

        // @brief Binds a callback for collision enter events.
        void addCollisionEnterBinding(std::function<void(entt::entity, entt::entity, const CollisionHit&)> callback) {
            onCollisionEnter = callback;
        }

        // @brief Binds a callback for collision stay events.
        void addCollisionStayBinding(std::function<void(entt::entity, entt::entity, const CollisionHit&)> callback) {
            onCollisionStay = callback;
        }

        // @brief Binds a callback for collision exit events.
        void addCollisionExitBinding(std::function<void(entt::entity, entt::entity)> callback) {
            onCollisionExit = callback;
        }
    };

    /// @brief Class representing a physics system.
    class PhysicsSystem {
    public:
        /// @brief Constructor, initializes physics system.
        PhysicsSystem(entt::registry& registry) : m_registry(registry) {}

        /// @brief Destructor, simply calls shutdown()
        ~PhysicsSystem();

        /// @brief Sets the debug renderer for the physics system.
        void setDebugRenderer(JPH::DebugRenderer* renderer);

        /// @brief Draws debug information for the physics system.
        void drawDebug(bool drawConstraints = true, bool drawWireframe = true);

        /// @brief Initializes jolts physics system.
        bool init(size_t maxBodies = 1024);

        /// @brief clears all physics objects
        void shutdown();

        /// @brief Updates physics, it is technically fixed time but needs delta to track if required time already passed
        void update(float deltaTime);

        /// @brief Scans registry for PhysicsComponents without bodies and creates them.
        void SyncBodies();

        /// @brief Allows for updating gravity.
        void SetGravityVector(const glm::vec3& gravity) {
            m_physicsSystem->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
        }

        /// @brief Allows for updating gravity factor per object.
        void SetGravityFactor(const JPH::BodyID& inBodyID, float inGravityFactor) {
            m_physicsSystem->GetBodyInterface().SetGravityFactor(inBodyID, inGravityFactor);
        }

        /// @brief Allows for updating friction.
        void SetFriction(JPH::BodyID bodyId, float friction);

        /// @brief Allows for getting friction.
        float GetFriction(JPH::BodyID bodyId);

        /// @brief Allows for updating bounciness.
        void SetBounciness(JPH::BodyID bodyId, float bounciness);

        /// @brief Allows for getting bounciness.
        float GetBounciness(JPH::BodyID bodyId);

        /// @brief Allows for updating linear velocity.
        void SetLinearVelocity(JPH::BodyID bodyId, const glm::vec3& velocity);

        /// @brief Allows for getting linear velocity.
        glm::vec3 GetLinearVelocity(JPH::BodyID bodyId);

        // @brief Gets the velocity of a specific point on the body (in World Space)
        glm::vec3 GetVelocityAtPosition(JPH::BodyID bodyId, const glm::vec3& point);

        /// @brief Allows for adding to linear velocity.
        void AddLinearVelocity(JPH::BodyID bodyId, const glm::vec3& velocity);

        /// @brief Allows for setting angular velocity.
        void SetAngularVelocity(JPH::BodyID bodyId, const glm::vec3& velocity);

        /// @brief Allows for getting angular velocity.
        glm::vec3 GetAngularVelocity(JPH::BodyID bodyId);

        /// @brief Allows for adding to angular velocity.
        void AddAngularVelocity(JPH::BodyID bodyId, const glm::vec3& velocity);

        /// @brief Applies torque to the body, it is reset next physics tick.
        void AddTorque(JPH::BodyID bodyId, const glm::vec3& torque);

        /// @brief Applies force at the mass center of the body, it is reset next physics tick.
        void AddForce(JPH::BodyID bodyId, const glm::vec3& force);

        /// @brief Applies force at the given position of the body, it is reset next physics tick.
        void AddForceAtPosition(JPH::BodyID bodyId, const glm::vec3& force, const glm::vec3& position);

        /// @brief Applies impulse at the mass center of the body, it is reset next physics tick.
        void AddImpulse(JPH::BodyID bodyId, const glm::vec3& impulse);

        /// @brief Applies impulse at the given position of the body, it is reset next physics tick.
        void AddImpulseAtPosition(JPH::BodyID bodyId, const glm::vec3& impulse, const glm::vec3& position);

        /// @brief Applies angular impulse at the mass center of the body, it is reset next physics tick.
        void AddAngularImpulse(JPH::BodyID bodyId, const glm::vec3& impulse);

        /// @brief Allows for setting body active state.
        void SetBodyActive(JPH::BodyID bodyId, bool active);

        /// @brief Allows for getting body active state.
        bool GetBodyActive(JPH::BodyID bodyId);

        /// @brief Allows to recreate physics body at runtime.
        std::optional<JPH::BodyID> CreateBodyForEntity(entt::entity e, entt::registry& r, PhysicsComponent& pc);

        /// @brief Allows to recreate physics body at runtime.
        std::optional<JPH::BodyID> RecreateBodyForEntity(entt::entity e, PhysicsComponent& pc);

        /// @brief Destroys a physics body.
        void DestroyBodyForEntity(PhysicsComponent& pc);

        /// @brief Performs a raycast in the physics world.
        bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit& hit);

        /// @brief Allows for getting collision steps (collision accuracy).
        /// @return int - number of collision steps
        int GetCollisionSteps() { return collisionSteps; }

        /// @brief Allows for setting collision steps (collision accuracy).
        /// @param int steps - number of collision steps
        void setCollisionSteps(int steps) { collisionSteps = steps; }

        // @brief Retrieves the physics component associated with a body ID.
        PhysicsComponent& getPhysicsComponentByBodyId(JPH::BodyID id);

        // @brief Retrieves the entity associated with a body ID.
        entt::entity getEntityByBodyId(JPH::BodyID id);

    private:
        friend class MyContactListener;

        int collisionSteps = 3;

        JPH::TempAllocatorImpl* m_tempAllocator = nullptr;
        JPH::JobSystem* m_jobSystem = nullptr;
        JPH::PhysicsSystem* m_physicsSystem = nullptr;
        JPH::DebugRenderer* m_debugRenderer = nullptr;

        entt::registry& m_registry;

        BPLayerInterfaceImpl m_bpInterface;
        ObjectVsBroadPhaseLayerFilterImpl m_objVsBpFilter;
        ObjectLayerPairFilterImpl m_objLayerPairFilter;
        std::unique_ptr<MyActivationListener> m_activationListener;
        std::unique_ptr<JPH::ContactListener> m_contactListener;

        std::unordered_map<JPH::BodyID, entt::entity, BodyIDHasher> m_bodyToEntity;

        entt::scoped_connection m_destroyConnection;

        float m_fixedDt = 1.0f / 60.0f;
        float m_accumulator = 0.0f;

        // @brief Converts Euler angles to a quaternion.
        static JPH::Quat EulerToQuat(const glm::vec3& eulerDeg);
        // @brief Converts a quaternion to Euler angles.
        static glm::vec3 QuatToEuler(const JPH::Quat& q);
        // @brief Synchronizes a body's transform with its entity's transform.
        void SyncBodyToTransform(entt::entity e, entt::registry& r, const JPH::BodyID& id);

        // @brief Handles physics component destruction.
        void onPhysicsComponentDestroy(entt::registry& reg, entt::entity e);

        // @brief Initializes a character component.
        void InitializeCharacter(entt::entity e, CharacterComponent& cc);

        #include <map>

        /// @brief Helper to weld vertices based on position ONLY (ignoring UVs/Normals which split render meshes)
        void WeldVertices(const std::vector<glm::vec3>& inVerts, const std::vector<uint32_t>& inIndices,
                          JPH::VertexList& outVerts, JPH::IndexedTriangleList& outTris);
    };

    class MyContactListener : public JPH::ContactListener {
    public:
        MyContactListener(PhysicsSystem& system) : m_system(system) {}

        // @brief Validates if a contact should be processed.
        JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override {
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        // @brief Called when a new contact is added.
        void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
        // @brief Called when a contact persists.
        void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
        // @brief Called when a contact is removed.
        void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

    private:
        PhysicsSystem& m_system;
    };
}
