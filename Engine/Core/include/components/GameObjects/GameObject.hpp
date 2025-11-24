/**
 *  @file   GameObject.hpp
 *  @brief  This file defines the GameObject class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
  #include <entt/entt.hpp>
  //#include <nlohmann/json.hpp>

  #include "Engine.hpp"
  #include "components/errorUtils.hpp"

  #include "components/GameComponents/BasicComponents.hpp"
  #include "../../../thirdparty/uuid/UUID.hpp"

  namespace vex {

      /// @brief Its base class for all game objects, eg. Player, Enemy, Weapon. Your Class needs to inherit from it.
      /// @details Example:
      /// @code
      /// class Player : public GameObject {
      /// public:
      ///     Player(Engine& engine, const std::string& name)
      ///         : GameObject(engine, name) {
      ///         // Initialize player-specific components
      ///     }
      /// };
      /// @endcode
      /// @details This class serves as the foundation for all game objects within the engine. It provides essential functionalities such as entity management, name registration, and basic component initialization.
  class GameObject {
  public:
    /// @brief Default constructor for GameObject. You need to call it from your derived class, it gives your object a unique identifier and registers it with the engine.
    GameObject(Engine& engine, const std::string& name)
        : m_engine(engine), m_entity(m_engine.getRegistry().create()), m_isValid(true)  {

        std::string tempName = name;

        auto view = engine.getRegistry().view<NameComponent>();
        for (auto entity : view) {
            auto meshComponent = view.get<NameComponent>(entity);
            if(meshComponent.name == tempName){
                log("Warning: Object with name: '%s already exists! All objects should have unique names.", tempName.c_str());
                UUID uuidGenerator;
                tempName = tempName + uuidGenerator.generate_uuid();
                log("Info: Object created with name: '%s', it is still recommended to not rely on this name for identification. It is different every app run.", tempName.c_str());
            }
        }
        m_engine.getRegistry().emplace<NameComponent>(m_entity, tempName);
      }

      /// @brief Destroys the GameObject and removes it from the engine's registry.
      void Destroy() {
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

          /// @brief Destruktor woła Destroy
          virtual ~GameObject() {
              Destroy();
          };

          GameObject(const GameObject&) = delete;
          GameObject& operator=(const GameObject&) = delete;

          /// @brief Pozwolenie na przenoszenie
          GameObject(GameObject&& other) noexcept
              : m_engine(other.m_engine), m_entity(other.m_entity), m_isValid(other.m_isValid) {
              other.m_entity = entt::null;
              other.m_isValid = false;
          }

      /// @brief virtual void function you override to implement custom behavior when the game starts.
      virtual void BeginPlay() {}
      /// @brief virtual void function you override to implement custom behavior when the game updates (eg. every frame).
      virtual void Update(float deltaTime) {}
      /// @brief Function that returns entity object, which is the entity associated with this GameObject. Its mostly needed for parenting and other engine internal usage.
      entt::entity GetEntity() const { return m_entity; }
      /// @brief Function that returns engine reference allowing your custom Object to use engine provided methods.
      /// @details Example usage:
      /// @code
      /// GetEngine().setResolutionMode(ResolutionMode::PS1_SHARP)
      /// @endcode
      /// @details You could technically just use m_engine but its highly discarded as it can lead to confusion and potential bugs.
      Engine& GetEngine() { return m_engine; }
      /// @brief Function that returns reference to a requested component.
      /// @return Requested component reference eg. InputComponent.
      template<typename T> T& GetComponent() { return m_engine.getRegistry().get<T>(m_entity); }
      /// @brief Function that adds a component to the GameObject.
      template<typename T> void AddComponent(const T& comp) { m_engine.getRegistry().emplace_or_replace<T>(m_entity, comp); }
      /// @brief Function that checks if the GameObject has a component.
      /// @return True if the GameObject has the component, false otherwise.
      template<typename T> bool HasComponent() const { return m_engine.getRegistry().any_of<T>(m_entity); }
      bool isValid() const { return m_isValid; }
      /// @brief Function that parent this Object to another GameObject. You need to pass another GameObject's entity.
      /// @param entt::entity entity - Entity of GameObject we want to parent to.
      /// @details Example usage:
      /// @code
      /// GameObject* parent = new GameObject();
      /// GameObject* child = new GameObject();
      /// child->ParentTo(parent->GetEntity());
      /// @endcode
      void ParentTo(entt::entity entity){
          GetComponent<TransformComponent>().setParent(entity);
      }
      protected:
      Engine& m_engine;
      entt::entity m_entity;
      bool m_isValid;
  };
  }
