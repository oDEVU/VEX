/**
 *  @file   CameraObject.hpp
 *  @brief  This file defines in engine CameraObject class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
  #include "GameObject.hpp"
  #include "components/GameComponents/BasicComponents.hpp"

  namespace vex {
/// @brief Basic camera object class. it only contains transform and camera components.
  class CameraObject : public GameObject {
  public:
      CameraObject(Engine& engine, const std::string& name)
          : GameObject(engine, name) {
          AddComponent(TransformComponent{engine.getRegistry()});
          AddComponent(CameraComponent{});
      }
  };
  }
