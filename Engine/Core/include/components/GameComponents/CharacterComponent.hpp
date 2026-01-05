#pragma once
#include "../JoltSafe.hpp"
#include <glm/glm.hpp>

namespace vex {

/// @brief Character component for player or NPC characters.
struct CharacterComponent {
    JPH::Ref<JPH::CharacterVirtual> character = nullptr;

    float standingHeight = 1.35f;
    float standingRadius = 0.3f;
    float mass = 70.0f;
    float maxSlopeAngle = 45.0f;

    glm::vec3 controlInput = glm::vec3(0.0f);
    float verticalVelocity = 0.0f;

    bool isInitialized() const { return character != nullptr; }
    bool isSupported() const { return character->IsSupported(); }
};

}
