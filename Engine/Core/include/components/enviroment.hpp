/**
 *  @file   enviroment.hpp
 *  @brief  This file defines enviroment struct.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <glm/glm.hpp>

namespace vex {
    /// @brief This struct is used to hold all enviroment settings. eg. shading details or lighting setup.
    struct enviroment{
      /// @brief Enables Gouraud shading.
      bool gourardShading = true;
      /// @brief Enables passive vertex jittering. (PS1 style jitter when camera is not moving)
      bool passiveVertexJitter = true;
      /// @brief Enables vertex snapping. (PS1 style jitter when camera is moving)
      bool vertexSnapping = true;
      /// @brief Enables affine texxture warping. (PS1 style texture warping in extreme camera angles)
      bool affineWarping = true;
      /// @brief Enables whole screen color quantization. (PS1 style color compresion artifacts)
      bool screenQuantization = true;
      /// @brief Enables CRT TVs artifacts.
      bool ntfsArtifacts = true;
      /// @brief Enables texture quantization. (PS1 style texture quantization)
      bool textureQuantization = true;
      /// @brief Enables screen dithering. (PS1 style dithering)
      bool screenDither = true;
      /// @brief Ambient light color
      glm::vec3 ambientLight = glm::vec3(1.0f);
      /// @brief Ambient light strength
      float ambientLightStrength = 0.1f;
      /// @brief Sun light color
      glm::vec3 sunLight = glm::vec3(1.0f, 1.0f, 1.0f);
      /// @brief Sun direction
      glm::vec3 sunDirection = glm::vec3(1.0f, 1.0f, 1.0f);
      /// @brief Background color (Esentially sky color if not implemented any skybox)
      glm::vec3 clearColor = glm::vec3(0.1f);
    };
}
