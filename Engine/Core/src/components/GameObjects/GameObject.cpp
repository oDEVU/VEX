// GameObject.cpp
#include "components/GameObjects/GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/SceneManager.hpp"
#include "components/Scene.hpp"
#include "../../../thirdparty/uuid/UUID.hpp"

namespace vex {

    GameObject::GameObject(Engine& engine, const std::string& name)
        : m_engine(engine), m_entity(m_engine.getRegistry().create()), m_isValid(true)
    {
        m_engine.getRegistry().add_or_replace<NameComponent>(m_entity, name);
        auto* scene = m_engine.getSceneManager()->GetScene(m_engine.getSceneManager()->getLastSceneName());
        if(scene){
            scene->RegisterGameObject(this);
        }
    }

    void GameObject::Destroy() {
        if (!m_isValid) return;

            log("[GameObject] Destroying entity ID: %d", (int)m_entity);

            if (m_engine.getRegistry().has<TransformComponent>(m_entity)) {
                 vex::View<TransformComponent> view(m_engine.getRegistry());
                 view.each([this](vex::Entity entity, TransformComponent& transform) {
                     if (transform.getParent() == m_entity) {
                         transform.setParent(vex::NULL_ENTITY);
                     }
                 });
                 m_engine.getRegistry().destroy(m_entity);
            }

            m_entity = vex::NULL_ENTITY;
            m_isValid = false;
        }

    GameObject::~GameObject() {
        Destroy();
    }
}