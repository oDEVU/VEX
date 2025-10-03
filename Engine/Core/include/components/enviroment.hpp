#pragma once

#include <glm/glm.hpp>

namespace vex {
    struct enviroment{
      bool gourardShading = true;
      bool passiveVertexJitter = false;
      bool vertexSnapping = true;
      bool affineWarping = true;
      bool colorQuantization = true;
      bool ntfsArtifacts = true;

      glm::vec3 ambientLight = glm::vec3(1.0f);
      float ambientLightStrength = 0.1f;
      glm::vec3 sunLight = glm::vec3(1.0f, 1.0f, 1.0f);
      glm::vec3 sunDirection = glm::vec3(1.0f, 1.0f, 1.0f);
    };
}
