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
    /// @param Engine& engine Reference to the engine instance.
    /// @param std::string name Unique name for the game object.
    GameObject(Engine& engine, const std::string& name);

    /// @brief Destroys the GameObject and removes it from the engine's registry.
    void Destroy();

    /// @brief Destruktor woła Destroy
    virtual ~GameObject();

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

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

      /// @brief Function that sets the type of the GameObject.
      /// @param type - Type of the GameObject.
      void setObjectType(const std::string& type) {
              objectType = type;
          }

      /// @brief Function that returns the type of the GameObject.
      /// @return Type of the GameObject.
      const std::string& getObjectType() const { return objectType; }

      protected:
      Engine& m_engine;
      entt::entity m_entity;
      bool m_isValid;
      std::string objectType;
  };
  }
