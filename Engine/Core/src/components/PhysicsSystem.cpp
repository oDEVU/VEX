#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL 1
#endif
#include <entt/entity/fwd.hpp>
#include <components/PhysicsSystem.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <components/errorUtils.hpp>
#include <thread>
#include <execution>

#include <components/JoltSafe.hpp>

#ifdef _WIN32
#pragma push_macro("max")
#undef max
#endif

namespace vex {

    bool PhysicsSystem::init(size_t maxBodies) {
        JPH::RegisterDefaultAllocator();

        if (JPH::Factory::sInstance == nullptr) {
            JPH::Factory::sInstance = new JPH::Factory();
        }

        JPH::RegisterTypes();

        /// @todo make it use max models from context
        m_tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

        m_jobSystem = new JPH::JobSystemThreadPool(
            1024,
            256,
            std::max(1u, std::thread::hardware_concurrency() - 1)
        );

        m_physicsSystem = new JPH::PhysicsSystem();

        m_physicsSystem->Init(
            static_cast<uint32_t>(maxBodies),
            0,
            1024,
            1024,
            m_bpInterface,
            m_objVsBpFilter,
            m_objLayerPairFilter
        );
        m_physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

        m_activationListener = std::make_unique<MyActivationListener>();
        m_physicsSystem->SetBodyActivationListener(m_activationListener.get());

        m_contactListener = std::make_unique<MyContactListener>(*this);
        m_physicsSystem->SetContactListener(m_contactListener.get());

        m_destroyConnection = m_registry.on_destroy<PhysicsComponent>().connect<&PhysicsSystem::onPhysicsComponentDestroy>(*this);

        return true;
    }

    PhysicsSystem::~PhysicsSystem() {
        shutdown();
    }

    void PhysicsSystem::shutdown() {
        //if (m_destroyConnection) m_destroyConnection.disconnect();

        try {
            if (m_physicsSystem) {
                auto view = m_registry.view<PhysicsComponent>();
                for (auto e : view) {
                    auto& pc = view.get<PhysicsComponent>(e);
                    DestroyBodyForEntity(pc);
                }
            }
        } catch (const std::exception& e) {
            handle_exception(e);
        }

        if (m_physicsSystem) {
            delete m_physicsSystem;
            m_physicsSystem = nullptr;
        }
        if (m_jobSystem) {
            delete m_jobSystem;
            m_jobSystem = nullptr;
        }
        if (m_tempAllocator) {
            delete m_tempAllocator;
            m_tempAllocator = nullptr;
        }
        JPH::UnregisterTypes();
        if (JPH::Factory::sInstance != nullptr) {
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }
    }

    void PhysicsSystem::setDebugRenderer(JPH::DebugRenderer* renderer) {
        m_debugRenderer = renderer;
    }

    void PhysicsSystem::drawDebug(bool drawConstraints, bool drawWireframe) {
        #ifdef DEBUG
            if (m_physicsSystem && m_debugRenderer) {
                JPH::BodyManager::DrawSettings settings;
                settings.mDrawShape = drawWireframe;
                settings.mDrawShapeWireframe = drawWireframe;
                m_physicsSystem->DrawBodies(settings, m_debugRenderer);

                if (drawConstraints) {
                    m_physicsSystem->DrawConstraints(m_debugRenderer);
                }
            }
            #endif
    }

    JPH::Quat GlmToJph(const glm::quat& q) {
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    void PhysicsSystem::SyncBodies() {
        #if DEBUG
            if (!m_physicsSystem) return;

            auto& bodyInterface = m_physicsSystem->GetBodyInterface();

            auto view = m_registry.view<PhysicsComponent, TransformComponent>();
            for (auto e : view) {
                auto& tc = view.get<TransformComponent>(e);
                auto& pc = view.get<PhysicsComponent>(e);

                if (pc.bodyId.GetIndexAndSequenceNumber() == JPH::BodyID::cInvalidBodyID) {
                    CreateBodyForEntity(e, m_registry, pc);
                    continue;
                }else if(pc.updated){
                    RecreateBodyForEntity(e, pc);
                    continue;
                }

                JPH::RVec3 pos(tc.getWorldPosition().x, tc.getWorldPosition().y, tc.getWorldPosition().z);
                JPH::Quat rot = GlmToJph(tc.getWorldQuaternion());
                bodyInterface.SetPositionAndRotation(pc.bodyId, pos, rot, JPH::EActivation::Activate);
            }
            #else
            log("This method is meant for debug builds only");
            #endif
        }

        void PhysicsSystem::InitializeCharacter(entt::entity e, CharacterComponent& cc) {
            if (!m_registry.all_of<TransformComponent>(e)) return;
            auto& tc = m_registry.get<TransformComponent>(e);

            JPH::Ref<JPH::Shape> shape = new JPH::CapsuleShape(
                0.5f * (cc.standingHeight - 2.0f * cc.standingRadius),
                cc.standingRadius
            );

            JPH::CharacterVirtualSettings settings;
            settings.mShape = shape;
            settings.mMaxSlopeAngle = JPH::DegreesToRadians(cc.maxSlopeAngle);
            settings.mMass = cc.mass;

            JPH::RVec3 pos(tc.getWorldPosition().x, tc.getWorldPosition().y, tc.getWorldPosition().z);
            JPH::Quat rot = GlmToJph(tc.getWorldQuaternion());

            cc.character = new JPH::CharacterVirtual(&settings, pos, rot, 0, m_physicsSystem);
        }

    void PhysicsSystem::update(float deltaTime) {
        if (!m_physicsSystem) return;

        auto charView = m_registry.view<CharacterComponent, TransformComponent>();
            for (auto e : charView) {
                auto& cc = charView.get<CharacterComponent>(e);
                auto& tc = charView.get<TransformComponent>(e);

                if (!cc.isInitialized()) {
                    InitializeCharacter(e, cc);
                }

                if (tc.transformedLately()) {
                    cc.character->SetPosition(JPH::RVec3(tc.getWorldPosition().x, tc.getWorldPosition().y, tc.getWorldPosition().z));
                }

                JPH::Vec3 currentVelocity = cc.character->GetLinearVelocity();

                float newVerticalVel = currentVelocity.GetY() + m_physicsSystem->GetGravity().GetY() * deltaTime;
                if (cc.character->IsSupported()) {
                    newVerticalVel = std::max(0.0f, newVerticalVel);
                }

                JPH::Vec3 finalVelocity(cc.controlInput.x, ( newVerticalVel + cc.controlInput.y ), cc.controlInput.z);

                cc.character->SetLinearVelocity(finalVelocity);

                JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
                JPH::ObjectLayer charLayer = 0;

                cc.character->ExtendedUpdate(
                    deltaTime,
                    m_physicsSystem->GetGravity(),
                    updateSettings,
                    m_physicsSystem->GetDefaultBroadPhaseLayerFilter(charLayer),
                    m_physicsSystem->GetDefaultLayerFilter(charLayer),
                    {},
                    {},
                    *m_tempAllocator
                );

                JPH::RVec3 newPos = cc.character->GetPosition();
                tc.setWorldPositionPhys(glm::vec3(newPos.GetX(), newPos.GetY(), newPos.GetZ()));

                tc.updatedPhysicsTransform();
                cc.controlInput = glm::vec3(0.0f);
            }

        auto& bodyInterface = m_physicsSystem->GetBodyInterface();
        auto view = m_registry.view<PhysicsComponent, TransformComponent>();
        for (auto e : view) {
            auto& tc = view.get<TransformComponent>(e);
            auto& pc = view.get<PhysicsComponent>(e);
            if (pc.bodyId.GetIndexAndSequenceNumber() == JPH::BodyID::cInvalidBodyID) {
                CreateBodyForEntity(e, m_registry, pc);
                continue;
            }
            if (tc.transformedLately()) {
                JPH::RVec3 pos(tc.getWorldPosition().x, tc.getWorldPosition().y, tc.getWorldPosition().z);
                JPH::Quat rot = GlmToJph(tc.getWorldQuaternion());
                    bodyInterface.SetPositionAndRotation(pc.bodyId, pos, rot, JPH::EActivation::Activate);
                tc.updatedPhysicsTransform();
            }
        }

        m_accumulator += deltaTime;

        while (m_accumulator >= m_fixedDt) {
            m_physicsSystem->Update(m_fixedDt, collisionSteps, m_tempAllocator, m_jobSystem);
            m_accumulator -= m_fixedDt;
        }

        //for (auto e : view) {
        if(view.size_hint() > 64){
            //log("--- mt ---");
            std::for_each(std::execution::par_unseq, view.begin(), view.end(), [&](auto e) {
                auto& pc = view.get<PhysicsComponent>(e);
                if (pc.bodyId.GetIndexAndSequenceNumber() != JPH::BodyID::cInvalidBodyID) {
                    SyncBodyToTransform(e, m_registry, pc.bodyId);
                }
            });
        }else{
            //log("--- st ---");
            for (auto e : view) {
                auto& pc = view.get<PhysicsComponent>(e);
                if (pc.bodyId.GetIndexAndSequenceNumber() != JPH::BodyID::cInvalidBodyID) {
                    SyncBodyToTransform(e, m_registry, pc.bodyId);
                }
            }
        }
    }

    std::optional<JPH::BodyID> PhysicsSystem::CreateBodyForEntity(entt::entity e, entt::registry& r, PhysicsComponent& pc) {
        if (!m_physicsSystem || !r.all_of<TransformComponent>(e)) return std::nullopt;

        auto& t = r.get<TransformComponent>(e);
        t.setPhysicsAffected(true);
        JPH::RVec3 pos(t.getWorldPosition().x, t.getWorldPosition().y, t.getWorldPosition().z);
        JPH::Quat rot = GlmToJph(t.getWorldQuaternion());

        JPH::ShapeRefC shape;
        switch (pc.shape) {
        case ShapeType::BOX:
            shape = new JPH::BoxShape(JPH::Vec3(pc.boxHalfExtents.x, pc.boxHalfExtents.y, pc.boxHalfExtents.z));
            break;
        case ShapeType::SPHERE:
            shape = new JPH::SphereShape(pc.sphereRadius);
            break;
        case ShapeType::CAPSULE:
            shape = new JPH::CapsuleShape(pc.capsuleHeight / 2.0f, pc.capsuleRadius);
            break;
        case ShapeType::CYLINDER:
            shape = new JPH::CylinderShape(pc.cylinderHeight / 2.0f, pc.cylinderRadius);
            break;
        case ShapeType::CONVEX_HULL:
            if (pc.convexPoints.empty()) {
                log(LogLevel::ERROR, "Convex hull shape has no points");
                return std::nullopt;
            }
            {
                JPH::Array<JPH::Vec3> vertices;
                vertices.reserve(pc.convexPoints.size()/3);
                for (const auto& p : pc.convexPoints) {
                    vertices.emplace_back(JPH::Vec3(p.GetX(), p.GetY(), p.GetZ()));
                }
                JPH::ConvexHullShapeSettings settings(vertices);
                shape = settings.Create().Get();
            }
            break;
        case ShapeType::MESH:
            if (pc.meshVertices.empty() || pc.meshIndices.empty()) {
                log(LogLevel::ERROR, "Mesh shape has no vertices or indices");
                return std::nullopt;
            }
            {
                JPH::IndexedTriangleList tris;
                tris.reserve(pc.meshIndices.size() / 3);
                for (size_t i = 0; i < pc.meshIndices.size(); i += 3) {
                    tris.emplace_back(pc.meshIndices[i], pc.meshIndices[i + 1], pc.meshIndices[i + 2]);
                }
                if (pc.bodyType == BodyType::DYNAMIC) {
                    JPH::Array<JPH::Vec3> verts;
                    verts.reserve(pc.convexPoints.size()/3);
                    for (const auto& p : pc.convexPoints) {
                        verts.emplace_back(JPH::Vec3(p.GetX(), p.GetY(), p.GetZ()));
                    }
                    log(LogLevel::WARNING, "Dynamic mesh fallback to convex hull");
                    JPH::ConvexHullShapeSettings settings(verts);
                    shape = settings.Create().Get();
                } else {
                    JPH::Array<JPH::Float3> verts;
                    verts.reserve(pc.meshVertices.size());
                    for (const auto& v : pc.meshVertices) {
                        verts.emplace_back(v.x, v.y, v.z);
                    }
                    JPH::MeshShapeSettings settings(verts, tris);
                    shape = settings.Create().Get();
                }
            }
            break;
        }

        JPH::EMotionType motion = JPH::EMotionType::Static;
        if (pc.bodyType == BodyType::DYNAMIC) motion = JPH::EMotionType::Dynamic;
        else if (pc.bodyType == BodyType::KINEMATIC) motion = JPH::EMotionType::Kinematic;
        else if (pc.bodyType == BodyType::SENSOR) motion = JPH::EMotionType::Dynamic;

        JPH::BodyCreationSettings settings(shape, pos, rot, motion, pc.objectLayer);
        settings.mLinearDamping = pc.linearDamping;
        settings.mAngularDamping = pc.angularDamping;
        settings.mAllowSleeping = pc.allowSleeping;
        settings.mIsSensor = pc.isSensor || pc.bodyType == BodyType::SENSOR;
        if (pc.bodyType == BodyType::DYNAMIC || pc.bodyType == BodyType::KINEMATIC) {
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
            settings.mMassPropertiesOverride.mMass = pc.mass;
        }

        auto& bodyInterface = m_physicsSystem->GetBodyInterface();
        JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
        if (bodyId.GetIndexAndSequenceNumber() == JPH::BodyID::cInvalidBodyID) return std::nullopt;

        bodyInterface.SetFriction(bodyId, pc.friction);
        bodyInterface.SetRestitution(bodyId, pc.bounce);

        pc.bodyId = bodyId;
        m_bodyToEntity[bodyId] = e;

        return bodyId;
    }

    std::optional<JPH::BodyID> PhysicsSystem::RecreateBodyForEntity(entt::entity e, PhysicsComponent& pc) {
        DestroyBodyForEntity(pc);
        return CreateBodyForEntity(e, m_registry, pc);
    }

    void PhysicsSystem::DestroyBodyForEntity(PhysicsComponent& pc) {
        if (!m_physicsSystem || pc.bodyId.GetIndexAndSequenceNumber() == JPH::BodyID::cInvalidBodyID) return;
        auto& bi = m_physicsSystem->GetBodyInterface();
        bi.RemoveBody(pc.bodyId);
        bi.DestroyBody(pc.bodyId);
        m_bodyToEntity.erase(pc.bodyId);
        pc.bodyId = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
    }

    void PhysicsSystem::onPhysicsComponentDestroy(entt::registry& reg, entt::entity e) {
        if (reg.all_of<PhysicsComponent>(e)) {
            auto& pc = reg.get<PhysicsComponent>(e);
            DestroyBodyForEntity(pc);
        }
    }

    JPH::Quat PhysicsSystem::EulerToQuat(const glm::vec3& eulerDeg) {
        glm::vec3 eulerRad = glm::radians(eulerDeg);
        glm::quat q = glm::yawPitchRoll(eulerRad.y, eulerRad.x, eulerRad.z);
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    glm::vec3 PhysicsSystem::QuatToEuler(const JPH::Quat& q) {
        glm::quat gq(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
        return glm::degrees(glm::eulerAngles(gq));
    }

    void PhysicsSystem::SyncBodyToTransform(entt::entity e, entt::registry& r, const JPH::BodyID& id) {
        auto& bi = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::RVec3 pos = bi.GetCenterOfMassPosition(id);
        JPH::Quat rot = bi.GetRotation(id);

        auto& t = r.get<TransformComponent>(e);
        t.setWorldPositionPhys(glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ()));
        t.setWorldQuaternion(glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ()));
    }

    PhysicsComponent& PhysicsSystem::getPhysicsComponentByBodyId(JPH::BodyID id) {
        auto it = m_bodyToEntity.find(id);
        if (it != m_bodyToEntity.end()) {
            return m_registry.get<PhysicsComponent>(it->second);
        }
        static PhysicsComponent dummy;
        return dummy;
    }

    entt::entity PhysicsSystem::getEntityByBodyId(JPH::BodyID id) {
        auto it = m_bodyToEntity.find(id);
        return it != m_bodyToEntity.end() ? it->second : entt::null;
    }

    bool PhysicsSystem::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit& hit) {
        if (!m_physicsSystem) return false;

        JPH::RVec3 start(origin.x, origin.y, origin.z);
        JPH::Vec3 dir(direction.x, direction.y, direction.z);
        JPH::RRayCast ray(start, dir * maxDistance);
        JPH::RayCastResult result;
        if (m_physicsSystem->GetNarrowPhaseQuery().CastRay(ray, result)) {
            hit.bodyId = result.mBodyID;
            hit.distance = result.mFraction * maxDistance;
            hit.position = origin + direction * hit.distance;
            hit.normal = glm::vec3(0.0f);  // Normal requires collector
            return true;
        }
        return false;
    }

    void PhysicsSystem::SetFriction(JPH::BodyID bodyId, float friction) {
        auto& bi = m_physicsSystem->GetBodyInterface();
        bi.SetFriction(bodyId, friction);
        getPhysicsComponentByBodyId(bodyId).friction = friction;
    }

    float PhysicsSystem::GetFriction(JPH::BodyID bodyId) {
        return m_physicsSystem->GetBodyInterface().GetFriction(bodyId);
    }

    void PhysicsSystem::SetBounciness(JPH::BodyID bodyId, float bounciness) {
        auto& bi = m_physicsSystem->GetBodyInterface();
        bi.SetRestitution(bodyId, bounciness);
        getPhysicsComponentByBodyId(bodyId).bounce = bounciness;
    }

    float PhysicsSystem::GetBounciness(JPH::BodyID bodyId) {
        return m_physicsSystem->GetBodyInterface().GetRestitution(bodyId);
    }

    void PhysicsSystem::SetLinearVelocity(JPH::BodyID bodyId, const glm::vec3& velocity) {
        m_physicsSystem->GetBodyInterface().SetLinearVelocity(bodyId, JPH::Vec3(velocity.x, velocity.y, velocity.z));
    }

    glm::vec3 PhysicsSystem::GetLinearVelocity(JPH::BodyID bodyId) {
        auto vel = m_physicsSystem->GetBodyInterface().GetLinearVelocity(bodyId);
        return {vel.GetX(), vel.GetY(), vel.GetZ()};
    }

    void PhysicsSystem::AddLinearVelocity(JPH::BodyID bodyId, const glm::vec3& velocity) {
        auto& bi = m_physicsSystem->GetBodyInterface();
        JPH::Vec3 current = bi.GetLinearVelocity(bodyId);
        bi.SetLinearVelocity(bodyId, current + JPH::Vec3(velocity.x, velocity.y, velocity.z));
    }

    void PhysicsSystem::SetAngularVelocity(JPH::BodyID bodyId, const glm::vec3& velocity) {
        m_physicsSystem->GetBodyInterface().SetAngularVelocity(bodyId, JPH::Vec3(velocity.x, velocity.y, velocity.z));
    }

    glm::vec3 PhysicsSystem::GetAngularVelocity(JPH::BodyID bodyId) {
        auto vel = m_physicsSystem->GetBodyInterface().GetAngularVelocity(bodyId);
        return {vel.GetX(), vel.GetY(), vel.GetZ()};
    }

    void PhysicsSystem::AddAngularVelocity(JPH::BodyID bodyId, const glm::vec3& velocity) {
        auto& bi = m_physicsSystem->GetBodyInterface();
        JPH::Vec3 current = bi.GetAngularVelocity(bodyId);
        bi.SetAngularVelocity(bodyId, current + JPH::Vec3(velocity.x, velocity.y, velocity.z));
    }

    void PhysicsSystem::AddTorque(JPH::BodyID bodyId, const glm::vec3& torque) {
        m_physicsSystem->GetBodyInterface().AddTorque(bodyId, JPH::Vec3(torque.x, torque.y, torque.z));
    }

    void PhysicsSystem::AddForce(JPH::BodyID bodyId, const glm::vec3& force) {
        m_physicsSystem->GetBodyInterface().AddForce(bodyId, JPH::Vec3(force.x, force.y, force.z));
    }

    void PhysicsSystem::AddForceAtPosition(JPH::BodyID bodyId, const glm::vec3& force, const glm::vec3& position) {
        m_physicsSystem->GetBodyInterface().AddForce(bodyId, JPH::Vec3(force.x, force.y, force.z), JPH::RVec3(position.x, position.y, position.z));
    }

    void PhysicsSystem::AddImpulse(JPH::BodyID bodyId, const glm::vec3& impulse) {
        m_physicsSystem->GetBodyInterface().AddImpulse(bodyId, JPH::Vec3(impulse.x, impulse.y, impulse.z));
    }

    void PhysicsSystem::AddImpulseAtPosition(JPH::BodyID bodyId, const glm::vec3& impulse, const glm::vec3& position) {
        m_physicsSystem->GetBodyInterface().AddImpulse(bodyId, JPH::Vec3(impulse.x, impulse.y, impulse.z), JPH::RVec3(position.x, position.y, position.z));
    }

    void PhysicsSystem::AddAngularImpulse(JPH::BodyID bodyId, const glm::vec3& impulse) {
        m_physicsSystem->GetBodyInterface().AddAngularImpulse(bodyId, JPH::Vec3(impulse.x, impulse.y, impulse.z));
    }

    void PhysicsSystem::SetBodyActive(JPH::BodyID bodyId, bool active) {
        auto& bi = m_physicsSystem->GetBodyInterface();
        active ? bi.ActivateBody(bodyId) : bi.DeactivateBody(bodyId);
    }

    bool PhysicsSystem::GetBodyActive(JPH::BodyID bodyId) {
        return m_physicsSystem->GetBodyInterface().IsActive(bodyId);
    }

    void MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
        entt::entity e1 = m_system.getEntityByBodyId(inBody1.GetID());
        entt::entity e2 = m_system.getEntityByBodyId(inBody2.GetID());
        CollisionHit hit;
        hit.position = glm::vec3(inManifold.mWorldSpaceNormal.GetX(), inManifold.mWorldSpaceNormal.GetY(), inManifold.mWorldSpaceNormal.GetZ());
        hit.normal = hit.position;
        hit.impulse = ioSettings.mCombinedRestitution;

        if (e1 != entt::null && m_system.m_registry.all_of<PhysicsComponent>(e1)) {
            auto& pc = m_system.m_registry.get<PhysicsComponent>(e1);
            if (pc.onCollisionEnter) pc.onCollisionEnter(e1, e2, hit);
        }
        if (e2 != entt::null && m_system.m_registry.all_of<PhysicsComponent>(e2)) {
            auto& pc = m_system.m_registry.get<PhysicsComponent>(e2);
            if (pc.onCollisionEnter) pc.onCollisionEnter(e2, e1, hit);
        }
    }

    void MyContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
        entt::entity e1 = m_system.getEntityByBodyId(inBody1.GetID());
        entt::entity e2 = m_system.getEntityByBodyId(inBody2.GetID());
        CollisionHit hit;
        hit.position = glm::vec3(inManifold.mWorldSpaceNormal.GetX(), inManifold.mWorldSpaceNormal.GetY(), inManifold.mWorldSpaceNormal.GetZ());
        hit.normal = hit.position;
        hit.impulse = ioSettings.mCombinedRestitution;

        if (e1 != entt::null && m_system.m_registry.all_of<PhysicsComponent>(e1)) {
            auto& pc = m_system.m_registry.get<PhysicsComponent>(e1);
            if (pc.onCollisionStay) pc.onCollisionStay(e1, e2, hit);
        }
        if (e2 != entt::null && m_system.m_registry.all_of<PhysicsComponent>(e2)) {
            auto& pc = m_system.m_registry.get<PhysicsComponent>(e2);
            if (pc.onCollisionStay) pc.onCollisionStay(e2, e1, hit);
        }
    }

    void MyContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) {
        JPH::BodyID id1 = inSubShapePair.GetBody1ID();
        JPH::BodyID id2 = inSubShapePair.GetBody2ID();
        entt::entity e1 = m_system.getEntityByBodyId(id1);
        entt::entity e2 = m_system.getEntityByBodyId(id2);

        if (e1 != entt::null && m_system.m_registry.all_of<PhysicsComponent>(e1)) {
            auto& pc = m_system.m_registry.get<PhysicsComponent>(e1);
            if (pc.onCollisionExit) pc.onCollisionExit(e1, e2);
        }
        if (e2 != entt::null && m_system.m_registry.all_of<PhysicsComponent>(e2)) {
            auto& pc = m_system.m_registry.get<PhysicsComponent>(e2);
            if (pc.onCollisionExit) pc.onCollisionExit(e2, e1);
        }
    }
}
