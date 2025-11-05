#define GLM_ENABLE_EXPERIMENTAL 1
#include "entt/entity/fwd.hpp"
#include <components/PhysicsSystem.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "components/errorUtils.hpp"
#include <thread>

#include <components/JoltSafe.hpp>

#ifdef _WIN32
#pragma push_macro("max")  // Save if needed
#undef max
#endif

namespace vex {

        bool PhysicsSystem::init(size_t maxBodies)
        {
            JPH::RegisterDefaultAllocator();

            if (JPH::Factory::sInstance == nullptr) {
                JPH::Factory::sInstance = new JPH::Factory();
            }

            JPH::RegisterTypes();

            m_tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

            m_jobSystem = new JPH::JobSystemThreadPool(
                1024,
                256,
                std::max(1u, std::thread::hardware_concurrency() - 1)
            );

            m_physicsSystem = new JPH::PhysicsSystem();

            static MyBroadPhaseLayerInterface broadPhaseLayerInterface;
            static MyObjectVsBroadPhaseLayerFilter objectVsBroadPhaseLayerFilter;
            static MyObjectLayerPairFilter objectLayerPairFilter;

            m_physicsSystem->Init(
                (uint32_t)maxBodies,
                0,
                1024,
                1024,
                broadPhaseLayerInterface,
                objectVsBroadPhaseLayerFilter,
                objectLayerPairFilter
            );
            m_physicsSystem->SetGravity(JPH::Vec3(0.0f, -1.81f, 0.0f));
            return true;
        }

    PhysicsSystem::~PhysicsSystem()
    {
        shutdown();
    }

    void PhysicsSystem::shutdown() {
        JPH::UnregisterTypes(); // optional but recommended

        // Delete factory singleton if we created it
        if (JPH::Factory::sInstance != nullptr) {
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }
        if (m_physicsSystem) {
            delete m_physicsSystem;
            m_physicsSystem = nullptr;
        }
        delete m_jobSystem;
        delete m_tempAllocator;
    }

    void PhysicsSystem::update(float deltaTime) {
        if (!m_physicsSystem) return;

        m_accumulator += deltaTime;
        while (m_accumulator >= m_fixedDt) {
            m_physicsSystem->Update(m_fixedDt, 1, m_tempAllocator, m_jobSystem);
            m_accumulator -= m_fixedDt;
        }
        auto view = m_registry.view<PhysicsComponent, TransformComponent>();
        for (auto e : view) {
            auto& pc = view.get<PhysicsComponent>(e);
            if (pc.bodyId.IsInvalid()){
                CreateBodyForEntity(e, m_registry, pc);
            }else{
                SyncBodyToTransform(e, m_registry, pc.bodyId);
            }
        }
    }

    std::optional<JPH::BodyID> PhysicsSystem::CreateBodyForEntity(entt::entity e, entt::registry& r, PhysicsComponent& pc) {
        if (!m_physicsSystem) return std::nullopt;
        if (!r.all_of<TransformComponent>(e)) return std::nullopt;

        auto& t = r.get<TransformComponent>(e);
        JPH::Vec3 pos(t.position.x, t.position.y, t.position.z);
        JPH::Quat rot = EulerToQuat(t.rotation);

        JPH::Ref<JPH::Shape> shape;
        if (pc.shape == ShapeType::BOX)
            shape = new JPH::BoxShape(JPH::Vec3(pc.boxHalfExtents.x, pc.boxHalfExtents.y, pc.boxHalfExtents.z));
        else if (pc.shape == ShapeType::SPHERE)
            shape = new JPH::SphereShape(pc.sphereRadius);
        else if (pc.shape == ShapeType::CAPSULE)
            shape = new JPH::CapsuleShape(pc.capsuleRadius, pc.capsuleHeight);
        else if (pc.shape == ShapeType::CYLINDER)
            shape = new JPH::CylinderShape(pc.cylinderRadius, pc.cylinderHeight);

        JPH::EMotionType motion = pc.isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic;
        JPH::BodyCreationSettings settings(shape, JPH::RVec3(pos), rot, motion, pc.objectLayer);
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = pc.mass;

        auto& bodyInterface = m_physicsSystem->GetBodyInterface();
        JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
        if (bodyId.IsInvalid()) return std::nullopt;

        bodyInterface.SetFriction(bodyId, pc.friction);
        bodyInterface.SetRestitution(bodyId, pc.bounce);

        pc.bodyId = bodyId;
        bodyInterface.ActivateBody(bodyId);
        return bodyId;
    }

    void PhysicsSystem::DestroyBodyForEntity(PhysicsComponent& pc) {
        if (!m_physicsSystem || pc.bodyId.IsInvalid()) return;
        auto& bi = m_physicsSystem->GetBodyInterface();
        bi.RemoveBody(pc.bodyId);
        bi.DestroyBody(pc.bodyId);
        pc.bodyId = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
    }

    JPH::Quat PhysicsSystem::EulerToQuat(const glm::vec3& eulerDeg) {
        glm::mat4 mat(1.0f);
        mat = glm::rotate(mat, glm::radians(eulerDeg.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Y first
        mat = glm::rotate(mat, glm::radians(eulerDeg.x), glm::vec3(1.0f, 0.0f, 0.0f)); // X
        mat = glm::rotate(mat, glm::radians(eulerDeg.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Z last
        glm::quat q = glm::quat_cast(mat);
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    glm::vec3 PhysicsSystem::QuatToEuler(const JPH::Quat& q) {
        glm::quat gq(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
        glm::mat4 mat = glm::mat4_cast(gq);
        float ex, ey, ez;
        glm::extractEulerAngleXYZ(mat, ex, ey, ez);

        //log("Quaterion: {%f,%f,%f,%f}", q.GetW(), q.GetX(), q.GetY(), q.GetZ());
        //log("Euler: {%f,%f,%f}", glm::degrees(ex), glm::degrees(ey), glm::degrees(ez));

        return glm::degrees(glm::vec3(ex, ey, ez));
    }

    void PhysicsSystem::SyncBodyToTransform(entt::entity e, entt::registry& r, const JPH::BodyID& id) {
        auto& bi = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::RVec3 pos = bi.GetPosition(id);
        JPH::Quat rot = bi.GetRotation(id);

        auto& t = r.get<TransformComponent>(e);
        t.position = {pos.GetX(), pos.GetY(), pos.GetZ()};
        t.rotation = QuatToEuler(rot);
    }

    PhysicsComponent& PhysicsSystem::getPhysicsComponentByBodyId(JPH::BodyID id){
        auto view = m_registry.view<PhysicsComponent>();
        for (auto e : view) {
            auto& pc = view.get<PhysicsComponent>(e);
            if (pc.bodyId == id){
                return pc;
            }
        }
        //return nullptr
    }
}
