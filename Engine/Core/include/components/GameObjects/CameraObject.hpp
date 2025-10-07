#pragma once
  #include "GameObject.hpp"
  #include "components/GameComponents/BasicComponents.hpp"

  namespace vex {
  class CameraObject : public GameObject {
  public:
      CameraObject(Engine& engine, const std::string& name)
          : GameObject(engine, name) {
          AddComponent(TransformComponent{});
          AddComponent(CameraComponent{});
      }
  };
  }
