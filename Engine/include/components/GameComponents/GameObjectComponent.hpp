#pragma once

#include "Engine.hpp"

  namespace vex {
      class GameObject;

      struct GameObjectComponent {
          std::unique_ptr<GameObject> object;
      };
  }
