// GameObject.cpp
#include "components/GameObjects/GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "../../../thirdparty/uuid/UUID.hpp"

namespace vex {

    GameObject::GameObject(Engine& engine, const std::string& name)
        : m_engine(engine), m_entity(m_engine.getRegistry().create()), m_isValid(true)
    {
        std::string tempName = name;

        // This logic now runs INSIDE VEX.DLL, using VEX's EnTT Type IDs
        auto view = engine.getRegistry().view<NameComponent>();
        for (auto entity : view) {
            const auto& comp = view.get<NameComponent>(entity);
            if (comp.name == tempName) {
                log("Warning: Duplicate object name '%s'.", tempName.c_str());
                // vex::UUID uuidGenerator; // Use namespace if you added one
                // tempName = tempName + uuidGenerator.generate_uuid();
                tempName = tempName + "_" + std::to_string((uint32_t)m_entity); // Simple fallback
            }
        }

        // CRITICAL: This now uses the Engine's ID for NameComponent
        m_engine.getRegistry().emplace<NameComponent>(m_entity, tempName);
    }

    void GameObject::Destroy() {
        if (!m_isValid) return;

            log("[GameObject] Destroying entity ID: %d", (int)m_entity);

            if (m_engine.getRegistry().valid(m_entity)) {
                 auto view = m_engine.getRegistry().view<TransformComponent>();
                 for (auto entity : view) {
                     auto& transform = view.get<TransformComponent>(entity);
                     if (transform.getParent() == m_entity) {
                         transform.setParent(entt::null);
                     }
                 }
                 m_engine.getRegistry().destroy(m_entity);
            }

            m_entity = entt::null;
            m_isValid = false;
        }

    GameObject::~GameObject() {
        Destroy();
    }
}
