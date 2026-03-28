#pragma once

#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/ComponentFactory.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace vex {

/// @brief Destroys the entity after a set amount of time. Useful for temporary effects.
struct LifetimeComponent {
    float lifespan = 5.0f;
    float timeElapsed = 0.0f;
};

/// @brief Moves an entity back and forth using a sine wave without physics overhead.
struct OscillatorComponent {
    glm::vec3 axis = {0.0f, 1.0f, 0.0f};
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float timeOffset = 0.0f;

    // Internal state
    float currentTime = 0.0f;
    glm::vec3 startPosition = {0.0f, 0.0f, 0.0f};
    bool initialized = false;
};

/// @brief Smoothly interpolates an entity's local position over time.
struct TweenComponent {
    glm::vec3 targetLocalOffset = {0.0f, 5.0f, 0.0f};
    float duration = 2.0f;
    bool pingPong = true;

    // Internal state
    float timeElapsed = 0.0f;
    glm::vec3 startPosition = {0.0f, 0.0f, 0.0f};
    bool initialized = false;
    bool returning = false;
};

} // namespace vex
