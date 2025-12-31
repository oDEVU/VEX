/**
 *  @file   FogObject.hpp
 *  @brief  This file defines in engine FogObject class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
  #include "GameObject.hpp"
  #include "components/GameComponents/BasicComponents.hpp"

  namespace vex {
/// @brief premade FogObject class.
  class FogObject : public GameObject {
  public:
      FogObject(Engine& engine, const std::string& name)
          : GameObject(engine, name) {
          AddComponent(FogComponent{});
      }
  };
  }
