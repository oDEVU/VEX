/**
 *  @file   PhysicsSystem.hpp
 *  @brief  This file defines PhysicsSystem class.
 *  @author Eryk Roszkowski
 ***********************************************/

 #pragma once

 #define GLM_ENABLE_EXPERIMENTAL 1
 #include <entt/entt.hpp>
 #include <glm/glm.hpp>
 #include <glm/gtc/matrix_transform.hpp>
 #include <glm/gtx/euler_angles.hpp>
 #include <glm/gtx/quaternion.hpp>
 #include <mutex>
 #include <optional>
 #include <vector>

 #include <components/GameComponents/BasicComponents.hpp>

 #include "components/JoltSafe.hpp"

 namespace vex{

     class MyBroadPhaseLayerInterface : public JPH::BroadPhaseLayerInterface {
     public:
         uint GetNumBroadPhaseLayers() const override { return 1; }
         JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override { return JPH::BroadPhaseLayer(0); }
     #if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
         const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override { return "Default"; }
     #endif
     };

     class MyObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter {
     public:
         bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override { return true; }
     };

     class MyObjectLayerPairFilter : public JPH::ObjectLayerPairFilter {
     public:
         bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override { return true; }
     };

     /// @brief Enumeration of available shape types for physics components.
     enum class ShapeType {
         BOX, SPHERE, CAPSULE, CYLINDER
     };

     /// @brief Structure representing a physics component.
     struct PhysicsComponent {
         ShapeType shape = ShapeType::BOX;
         bool isStatic = false;
         float mass = 1.0f;
         float friction = 0.5f;
         uint8_t objectLayer = 0;
         JPH::BodyID bodyId = JPH::BodyID(JPH::BodyID::cInvalidBodyID);

         glm::vec3 boxHalfExtents = {0.5f, 0.5f, 0.5f};
         float sphereRadius = 0.5f;
         float capsuleRadius = 0.5f;
         float capsuleHeight = 1.0f;
         float cylinderRadius = 0.5f;
         float cylinderHeight = 1.0f;

         /// --- Constructors ---
         PhysicsComponent() = default;

         explicit PhysicsComponent(ShapeType s) : shape(s) {}

         static PhysicsComponent Box(glm::vec3 halfExtents = {0.5f, 0.5f, 0.5f}, bool isStatic = false, float mass = 1.0f, float friction = 0.5f) {
             PhysicsComponent pc;
             pc.shape = ShapeType::BOX;
             pc.boxHalfExtents = halfExtents;
             pc.mass = mass;
             pc.isStatic = isStatic;
             pc.friction = friction;
             return pc;
         }

         static PhysicsComponent Sphere(float radius = 0.5f, bool isStatic = false, float mass = 1.0f, float friction = 0.5f) {
             PhysicsComponent pc;
             pc.shape = ShapeType::SPHERE;
             pc.sphereRadius = radius;
             pc.mass = mass;
             pc.isStatic = isStatic;
             pc.friction = friction;
             return pc;
         }

         static PhysicsComponent Capsule(float radius = 0.5f, float height = 1.0f, bool isStatic = false, float mass = 1.0f, float friction = 0.5f) {
             PhysicsComponent pc;
             pc.shape = ShapeType::CAPSULE;
             pc.capsuleRadius = radius;
             pc.capsuleHeight = height;
             pc.mass = mass;
             pc.isStatic = isStatic;
             pc.friction = friction;
             return pc;
         }

         static PhysicsComponent Cylinder(float radius = 0.5f, float height = 1.0f, bool isStatic = false, float mass = 1.0f, float friction = 0.5f) {
             PhysicsComponent pc;
             pc.shape = ShapeType::CYLINDER;
             pc.cylinderRadius = radius;
             pc.cylinderHeight = height;
             pc.mass = mass;
             pc.isStatic = isStatic;
             pc.friction = friction;
             return pc;
         }
     };

     /// @brief Class representing a physics system.
     class PhysicsSystem{
         public:

         /// @brief Destructor, simply calls shutdown()
         ~PhysicsSystem();

         /// @brief Initializes jolts physics system.
         bool init(size_t maxBodies = 1024);

         /// @brief clears all physics objects
         void shutdown();

         /// @brief Updates physics, it is technically fixed time but needs delta to track if required time already passed
         void update(float deltaTime, entt::registry& registry);

         /// @brief Allows for updating gravity.
         void SetGravityVector(const glm::vec3& gravity){
             m_physicsSystem->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
         }

         std::optional<JPH::BodyID> CreateBodyForEntity(entt::entity e, entt::registry& r, PhysicsComponent& pc);
         void DestroyBodyForEntity(PhysicsComponent& pc);

         private:
             JPH::TempAllocatorImpl* m_tempAllocator = nullptr;
             JPH::JobSystem* m_jobSystem = nullptr;
             JPH::PhysicsSystem* m_physicsSystem = nullptr;

             std::unique_ptr<JPH::BroadPhaseLayerInterface> m_bpInterface;
             std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_objVsBpFilter;
             std::unique_ptr<JPH::ObjectLayerPairFilter> m_objLayerPairFilter;

             float m_fixedDt = 1.0f / 60.0f;
             float m_accumulator = 0.0f;

             static JPH::Quat EulerToQuat(const glm::vec3& eulerDeg);
             static glm::vec3 QuatToEuler(const JPH::Quat& q);
             void SyncBodyToTransform(entt::entity e, entt::registry& r, const JPH::BodyID& id);

     };
 }
