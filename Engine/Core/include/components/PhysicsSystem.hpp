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

    /// @brief Hasher for JPH::BodyID
    struct BodyIDHasher {
        std::size_t operator()(const JPH::BodyID& id) const {
            return std::hash<uint32_t>{}(id.GetIndexAndSequenceNumber());
        }
    };

    /// @brief Implementation of JPH::BroadPhaseLayerInterface for the default layer.
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

    /// @brief Implementation of JPH::ObjectVsBroadPhaseLayerFilter for the default layer.
    class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        // @brief Determines if an object layer should collide with a broad phase layer.
        bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override { return true; }
    };

    /// @brief Implementation of JPH::ObjectLayerPairFilter for the default layer.
    class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
    public:
        // @brief Determines if two object layers should collide.
        bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override { return true; }
    };

    /// @brief Implementation of JPH::BodyActivationListener for the default layer.
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
        // @param glm::vec3 halfExtents The half-extents of the box.
        // @param BodyType bodyType The type of body (static, dynamic, kinematic).
        // @param float mass The mass of the box.
        // @param float friction The friction coefficient of the box.
        // @param float bounce The bounce coefficient of the box.
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
        // @param glm::vec3 halfExtents The half-extents of the rounded box.
        // @param float radius The radius of the rounded corners.
        // @param BodyType bodyType The type of body (static, dynamic, kinematic).
        // @param float mass The mass of the rounded box.
        // @param float friction The friction coefficient of the rounded box.
        // @param float bounce The bounce coefficient of the rounded box.
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
        // @param float radius The radius of the sphere.
        // @param BodyType bodyType The type of body (static, dynamic, kinematic).
        // @param float mass The mass of the sphere.
        // @param float friction The friction coefficient of the sphere.
        // @param float bounce The bounce coefficient of the sphere.
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
        // @param float radius The radius of the capsule.
        // @param float height The height of the capsule.
        // @param BodyType bodyType The type of body (static, dynamic, kinematic).
        // @param float mass The mass of the capsule.
        // @param float friction The friction coefficient of the capsule.
        // @param float bounce The bounce coefficient of the capsule.
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
        // @param float radius The radius of the cylinder.
        // @param float height The height of the cylinder.
        // @param BodyType bodyType The type of body (static, dynamic, kinematic).
        // @param float mass The mass of the cylinder.
        // @param float friction The friction coefficient of the cylinder.
        // @param float bounce The bounce coefficient of the cylinder.
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
        // @param std::vector<JPH::Vec3> points The points defining the convex hull.
        // @param BodyType bodyType The type of body (static, dynamic, kinematic).
        // @param float mass The mass of the convex hull.
        // @param float friction The friction coefficient of the convex hull.
        // @param float bounce The bounce coefficient of the convex hull.
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
        // @param MeshComponent& mesh The mesh component to create the physics component from.
        // @param BodyType bodyType The type of body (static, dynamic, kinematic).
        // @param float mass The mass of the mesh.
        // @param float friction The friction coefficient of the mesh.
        // @param float bounce The bounce coefficient of the mesh.
        static PhysicsComponent Mesh(MeshComponent& mesh, BodyType bodyType = BodyType::STATIC, float mass = 1.0f, float friction = 0.5f, float bounce = 0.1f) {
            PhysicsComponent pc;
            pc.shape = ShapeType::MESH;
            pc.bodyType = bodyType;
            pc.mass = mass;
            pc.friction = friction;
            pc.bounce = bounce;
            if (!mesh.meshData.vertices.empty()) {
                pc.meshVertices.clear();
                pc.meshIndices.clear();

                size_t vertexOffset = 0;

                //for (const auto& sm : mesh.meshData.submeshes) {
                    for (const auto& v : mesh.meshData.vertices) {
                        pc.meshVertices.push_back(v.position);
                    }
                    for (auto idx : mesh.meshData.indices) {
                        pc.meshIndices.push_back(static_cast<uint32_t>(idx + vertexOffset));
                    }
                    vertexOffset += mesh.meshData.vertices.size();
                    //}
            }
            return pc;
        }

        // @brief Binds a callback for collision enter events.
        // @param std::function<void(entt::entity, entt::entity, const CollisionHit&)> callback The callback function to bind.
        void addCollisionEnterBinding(std::function<void(entt::entity, entt::entity, const CollisionHit&)> callback) {
            onCollisionEnter = callback;
        }

        // @brief Binds a callback for collision stay events.
        // @param std::function<void(entt::entity, entt::entity, const CollisionHit&)> callback The callback function to bind.
        void addCollisionStayBinding(std::function<void(entt::entity, entt::entity, const CollisionHit&)> callback) {
            onCollisionStay = callback;
        }

        // @brief Binds a callback for collision exit events.
        // @param std::function<void(entt::entity, entt::entity)> callback The callback function to bind.
        void addCollisionExitBinding(std::function<void(entt::entity, entt::entity)> callback) {
            onCollisionExit = callback;
        }
    };

    /// @brief Class representing a physics system.
    class PhysicsSystem {
    public:
        /// @brief Constructor, initializes physics system.
        /// @param entt::registry& registry The registry to use for entity-component system.
        PhysicsSystem(entt::registry& registry) : m_registry(registry) {}

        /// @brief Destructor, simply calls shutdown()
        ~PhysicsSystem();

        /// @brief Sets the debug renderer for the physics system.
        /// @param JPH::DebugRenderer* renderer The debug renderer to use.
        void setDebugRenderer(JPH::DebugRenderer* renderer);

        /// @brief Draws debug information for the physics system.
        /// @param bool drawConstraints Whether to draw constraints.
        /// @param bool drawWireframe Whether to draw wireframe.
        void drawDebug(bool drawConstraints = true, bool drawWireframe = true);

        /// @brief Initializes jolts physics system.
        /// @param size_t maxBodies The maximum number of bodies to support.
        bool init(size_t maxBodies = 1024);

        /// @brief clears all physics objects
        void shutdown();

        /// @brief Updates physics, it is technically fixed time but needs delta to track if required time already passed
        /// @param float deltaTime The time elapsed since the last update.
        void update(float deltaTime);

        /// @brief Scans registry for PhysicsComponents without bodies and creates them.
        void SyncBodies();

        /// @brief Allows for updating gravity.
        /// @param const glm::vec3& gravity The new gravity vector.
        void SetGravityVector(const glm::vec3& gravity) {
            m_physicsSystem->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
        }

        /// @brief Allows for updating gravity factor per object.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param float gravityFactor The new gravity factor.
        void SetGravityFactor(JPH::BodyID bodyId, float gravityFactor) {
            m_physicsSystem->GetBodyInterface().SetGravityFactor(bodyId, gravityFactor);
        }

        /// @brief Allows for updating friction.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param float friction The new friction value.
        void SetFriction(JPH::BodyID bodyId, float friction);

        /// @brief Allows for getting friction.
        /// @param JPH::BodyID bodyId The ID of the body to get.
        float GetFriction(JPH::BodyID bodyId);

        /// @brief Allows for updating bounciness.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param float bounciness The new bounciness value.
        void SetBounciness(JPH::BodyID bodyId, float bounciness);

        /// @brief Allows for getting bounciness.
        /// @param JPH::BodyID bodyId The ID of the body to get.
        float GetBounciness(JPH::BodyID bodyId);

        /// @brief Allows for updating linear velocity.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& velocity The new linear velocity.
        void SetLinearVelocity(JPH::BodyID bodyId, const glm::vec3& velocity);

        /// @brief Allows for getting linear velocity.
        /// @param JPH::BodyID bodyId The ID of the body to get.
        glm::vec3 GetLinearVelocity(JPH::BodyID bodyId);

        /// @brief Gets the velocity of a specific point on the body (in World Space)
        /// @param JPH::BodyID bodyId The ID of the body to get.
        /// @param const glm::vec3& point The point on the body to get the velocity at.
        glm::vec3 GetVelocityAtPosition(JPH::BodyID bodyId, const glm::vec3& point);

        /// @brief Allows for adding to linear velocity.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& velocity The velocity to add.
        void AddLinearVelocity(JPH::BodyID bodyId, const glm::vec3& velocity);

        /// @brief Allows for setting angular velocity.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& velocity The velocity to set.
        void SetAngularVelocity(JPH::BodyID bodyId, const glm::vec3& velocity);

        /// @brief Allows for getting angular velocity.
        /// @param JPH::BodyID bodyId The ID of the body to get.
        glm::vec3 GetAngularVelocity(JPH::BodyID bodyId);

        /// @brief Allows for adding to angular velocity.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& velocity The velocity to add.
        void AddAngularVelocity(JPH::BodyID bodyId, const glm::vec3& velocity);

        /// @brief Applies torque to the body, it is reset next physics tick.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& torque The torque to apply.
        void AddTorque(JPH::BodyID bodyId, const glm::vec3& torque);

        /// @brief Applies force at the mass center of the body, it is reset next physics tick.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& force The force to apply.
        void AddForce(JPH::BodyID bodyId, const glm::vec3& force);

        /// @brief Applies force at the given position of the body, it is reset next physics tick.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& force The force to apply.
        /// @param const glm::vec3& position The position to apply the force at.
        void AddForceAtPosition(JPH::BodyID bodyId, const glm::vec3& force, const glm::vec3& position);

        /// @brief Applies impulse at the mass center of the body, it is reset next physics tick.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& impulse The impulse to apply.
        void AddImpulse(JPH::BodyID bodyId, const glm::vec3& impulse);

        /// @brief Applies impulse at the given position of the body, it is reset next physics tick.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& impulse The impulse to apply.
        /// @param const glm::vec3& position The position to apply the impulse at.
        void AddImpulseAtPosition(JPH::BodyID bodyId, const glm::vec3& impulse, const glm::vec3& position);

        /// @brief Applies angular impulse at the mass center of the body, it is reset next physics tick.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param const glm::vec3& impulse The impulse to apply.
        void AddAngularImpulse(JPH::BodyID bodyId, const glm::vec3& impulse);

        /// @brief Allows for setting body active state.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @param bool active The active state to set.
        void SetBodyActive(JPH::BodyID bodyId, bool active);

        /// @brief Allows for getting body active state.
        /// @param JPH::BodyID bodyId The ID of the body to update.
        /// @return bool The active state of the body.
        bool GetBodyActive(JPH::BodyID bodyId);

        /// @brief Allows to recreate physics body at runtime.
        /// @param entt::entity e The entity to create the body for.
        /// @param entt::registry& r The registry to get the entity data from.
        /// @param PhysicsComponent& pc The physics component to update.
        /// @return std::optional<JPH::BodyID> The ID of the created body.
        std::optional<JPH::BodyID> CreateBodyForEntity(entt::entity e, entt::registry& r, PhysicsComponent& pc);

        /// @brief Allows to recreate physics body at runtime.
        /// @param entt::entity e The entity to create the body for.
        /// @param PhysicsComponent& pc The physics component to update.
        /// @return std::optional<JPH::BodyID> The ID of the created body.
        std::optional<JPH::BodyID> RecreateBodyForEntity(entt::entity e, PhysicsComponent& pc);

        /// @brief Destroys a physics body.
        /// @param PhysicsComponent& pc The physics component to update.
        void DestroyBodyForEntity(PhysicsComponent& pc);

        /// @brief Performs a raycast in the physics world.
        /// @param const glm::vec3& origin The origin of the ray.
        /// @param const glm::vec3& direction The direction of the ray.
        /// @param float maxDistance The maximum distance to check.
        /// @param RaycastHit& hit The hit information.
        /// @return bool - true if a hit occurred, false otherwise.
        bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit& hit);

        /// @brief Allows for getting collision steps (collision accuracy).
        /// @return int - number of collision steps
        int GetCollisionSteps() { return collisionSteps; }

        /// @brief Allows for setting collision steps (collision accuracy).
        /// @param int steps - number of collision steps
        void setCollisionSteps(int steps) { collisionSteps = steps; }

        // @brief Retrieves the physics component associated with a body ID.
        // @param JPH::BodyID id - the body ID to retrieve the physics component for
        // @return PhysicsComponent& - the physics component associated with the body ID
        PhysicsComponent& getPhysicsComponentByBodyId(JPH::BodyID id);

        // @brief Retrieves the entity associated with a body ID.
        // @param JPH::BodyID id - the body ID to retrieve the entity for
        // @return entt::entity - the entity associated with the body ID
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
        // @param glm::vec3 eulerDeg - the Euler angles in degrees
        // @return JPH::Quat - the resulting quaternion
        static JPH::Quat EulerToQuat(const glm::vec3& eulerDeg);
        // @brief Converts a quaternion to Euler angles.
        // @param JPH::Quat q - the quaternion to convert
        // @return glm::vec3 - the resulting Euler angles in degrees
        static glm::vec3 QuatToEuler(const JPH::Quat& q);
        // @brief Synchronizes a body's transform with its entity's transform.
        // @param entt::entity e - the entity to synchronize
        // @param entt::registry& r - the registry containing the entity
        // @param const JPH::BodyID& id - the ID of the body to synchronize
        void SyncBodyToTransform(entt::entity e, entt::registry& r, const JPH::BodyID& id);

        // @brief Handles physics component destruction.
        // @param entt::registry& reg - the registry containing the entity
        // @param entt::entity e - the entity to destroy
        void onPhysicsComponentDestroy(entt::registry& reg, entt::entity e);

        // @brief Initializes a character component.
        // @param entt::entity e - the entity to initialize
        // @param CharacterComponent& cc - the character component to initialize
        void InitializeCharacter(entt::entity e, CharacterComponent& cc);

        #include <map>

        /// @brief Helper to weld vertices based on position ONLY (ignoring UVs/Normals which split render meshes)
        /// @param const std::vector<glm::vec3>& inVerts - the input vertices
        /// @param const std::vector<uint32_t>& inIndices - the input indices
        /// @param JPH::VertexList& outVerts - the output vertices
        /// @param JPH::IndexedTriangleList& outTris - the output triangles
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
        // @param const JPH::Body& inBody1 - the first body involved in the contact
        // @param const JPH::Body& inBody2 - the second body involved in the contact
        // @param const JPH::ContactManifold& inManifold - the contact manifold
        // @param JPH::ContactSettings& ioSettings - the contact settings
        void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
        // @brief Called when a contact persists.
        // @param const JPH::Body& inBody1 - the first body involved in the contact
        // @param const JPH::Body& inBody2 - the second body involved in the contact
        // @param const JPH::ContactManifold& inManifold - the contact manifold
        // @param JPH::ContactSettings& ioSettings - the contact settings
        void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
        // @brief Called when a contact is removed.
        // @param const JPH::SubShapeIDPair& inSubShapePair - the sub shape pair involved in the contact
        void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

    private:
        PhysicsSystem& m_system;
    };
}
