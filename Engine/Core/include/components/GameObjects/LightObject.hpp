/**
 *  @file   LightObject.hpp
 *  @brief  This file defines in engine LightObject class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
  #include "GameObject.hpp"
  #include "components/GameComponents/BasicComponents.hpp"

  namespace vex {
/// @brief premade LightObject class.
  class LightObject : public GameObject {
  public:
      LightObject(Engine& engine, const std::string& name)
          : GameObject(engine, name) {
          AddComponent(TransformComponent{});
          AddComponent(LightComponent{});
      }
  };
  }
