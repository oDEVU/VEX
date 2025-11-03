#define GLM_ENABLE_EXPERIMENTAL 1
#include "entt/entity/fwd.hpp"
#include <components/PhysicsSystem.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <components/JoltSafe.hpp>

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
                /*maxJobs=*/ 1024,
                /*maxBarriers=*/ 256,
                /*numThreads=*/ std::max(1u, std::thread::hardware_concurrency() - 1)
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
            m_physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
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

    void PhysicsSystem::update(float deltaTime, entt::registry& registry) {
        if (!m_physicsSystem) return;

        m_accumulator += deltaTime;
        while (m_accumulator >= m_fixedDt) {
            m_physicsSystem->Update(m_fixedDt, 1, m_tempAllocator, m_jobSystem);
            m_accumulator -= m_fixedDt;
        }

        auto view = registry.view<PhysicsComponent, TransformComponent>();
        for (auto e : view) {
            auto& pc = view.get<PhysicsComponent>(e);
            if (pc.bodyId.IsInvalid()){
                CreateBodyForEntity(e, registry, pc);
            }
            SyncBodyToTransform(e, registry, pc.bodyId);
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
        else
            shape = new JPH::SphereShape(pc.sphereRadius);

        JPH::EMotionType motion = pc.isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic;
        JPH::BodyCreationSettings settings(shape, JPH::RVec3(pos), rot, motion, pc.objectLayer);
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = pc.mass;

        auto& bodyInterface = m_physicsSystem->GetBodyInterface();
        JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
        if (bodyId.IsInvalid()) return std::nullopt;

        pc.bodyId = bodyId;
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
        glm::vec3 rad = glm::radians(eulerDeg);
        glm::quat q = glm::quat(rad); // glm builds from Euler (XYZ)
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    glm::vec3 PhysicsSystem::QuatToEuler(const JPH::Quat& q) {
        glm::quat gq(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
        glm::vec3 eulerRad = glm::eulerAngles(gq);
        return glm::degrees(eulerRad);
    }

    void PhysicsSystem::SyncBodyToTransform(entt::entity e, entt::registry& r, const JPH::BodyID& id) {
        auto& bi = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::RVec3 pos = bi.GetPosition(id);
        JPH::Quat rot = bi.GetRotation(id);

        auto& t = r.get<TransformComponent>(e);
        t.position = {pos.GetX(), pos.GetY(), pos.GetZ()};
        t.rotation = QuatToEuler(rot);
    }
}
