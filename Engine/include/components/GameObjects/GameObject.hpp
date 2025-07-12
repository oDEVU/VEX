#pragma once
  #include <entt/entt.hpp>
  #include <nlohmann/json.hpp>

  #include "Engine.hpp"
  #include "components/errorUtils.hpp"

  #include "components/GameComponents/BasicComponents.hpp"
  #include "components/GameComponents/GameObjectComponent.hpp"

  namespace vex {

  class GameObject {
  public:
      GameObject(Engine& engine, const std::string& name)
          : m_engine(engine), m_entity(m_engine.getRegistry().create()) {

        auto view = engine.getRegistry().view<NameComponent>();
        for (auto entity : view) {
            auto meshComponent = view.get<NameComponent>(entity);
            if(meshComponent.name == name){
                throw_error("Object with name: '" + name + "' already exists");
            }
        }
        m_engine.getRegistry().emplace<NameComponent>(m_entity, name);
        m_engine.getRegistry().emplace<GameObjectComponent>(m_entity, std::unique_ptr<GameObject>(this));
      }
    ~GameObject(){
        m_engine.getRegistry().destroy(m_entity);
    };
      virtual void BeginPlay() {}
      virtual void Update(float deltaTime) {}
      //virtual void SetParameters(const nlohmann::json& params) {}
      entt::entity GetEntity() const { return m_entity; }
      Engine& GetEngine() { return m_engine; }
      template<typename T> T& GetComponent() { return m_engine.getRegistry().get<T>(m_entity); }
      template<typename T> void AddComponent(const T& comp) { m_engine.getRegistry().emplace_or_replace<T>(m_entity, comp); }
      template<typename T> bool HasComponent() const { return m_engine.getRegistry().any_of<T>(m_entity); }
  protected:
      Engine& m_engine;
      entt::entity m_entity;
  };
  }
