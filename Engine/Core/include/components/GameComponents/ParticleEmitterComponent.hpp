/**
 *  @file   ParticleEmitterComponent.hpp
 *  @brief  Particle emitter component
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "components/AssetTypes.hpp"
#include "components/ColorTypes.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace vex {

    /// @brief Structure passed to the GPU containing per-particle rendering data.
    struct alignas(16) ParticleGPUData {
        glm::vec4 position; ///< Particle position (xyz) and unused (w).
        float scaleX;       ///< Particle horizontal scale.
        float scaleY;       ///< Particle vertical scale.
        uint32_t textureID; ///< Bindless texture index.
        uint32_t isUnlit;   ///< Flag determining if the particle is unlit (1) or affected by lighting (0).
        glm::vec4 color;    ///< Particle color (rgba).
    };



    /// @brief CPU-side representation of a single particle during its lifetime.
    struct Particle {
        glm::vec3 position = glm::vec3(0.0f); ///< Current world position.
        glm::vec3 velocity = glm::vec3(0.0f); ///< Current velocity.
        vex::rgba color = vex::rgba(1.0f, 1.0f, 1.0f, 1.0f); ///< Current color.
        vex::rgba startColor = vex::rgba(1.0f, 1.0f, 1.0f, 1.0f); ///< Color at spawn.
        vex::rgba endColor = vex::rgba(1.0f, 1.0f, 1.0f, 0.0f); ///< Color at end of life.
        float startSize = 1.0f; ///< Size at spawn.
        float endSize = 0.0f;   ///< Size at end of life.
        float life = 0.0f;      ///< Remaining lifetime in seconds.
        float startingLife = 1.0f; ///< Total lifetime in seconds.
    };

    /// @brief Component that emits and manages a system of particles.
    struct ParticleEmitterComponent {
        vex::texture_asset_path texturePath; ///< Path to the texture applied to particles.
        bool isTransparent = true;  ///< Indicates if the particles should be rendered in the transparent pass.
        bool isUnlit = true;        ///< Indicates if the particles should ignore lighting.
        float spawnRate = 0.1f;     ///< Time in seconds between particle spawns.
        float spawnTimer = 0.0f;    ///< Internal timer to track time since last spawn.
        glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f); ///< Gravity applied to particles.
        glm::vec3 initialVelocity = glm::vec3(0.0f, 5.0f, 0.0f); ///< Base starting velocity for new particles.
        glm::vec3 velocityVariation = glm::vec3(2.0f, 2.0f, 2.0f); ///< Random variation range applied to initial velocity.
        float particleLife = 3.0f;  ///< Base lifetime for new particles.
        float particleLifeVariation = 0.5f; ///< Random variation range applied to lifetime.
        float startSize = 1.0f;     ///< Base size for new particles.
        float endSize = 0.0f;       ///< Size of particles at the end of their life.
        vex::rgba startColor = vex::rgba(1.0f, 1.0f, 1.0f, 1.0f); ///< Color of particles at spawn.
        vex::rgba endColor = vex::rgba(1.0f, 1.0f, 1.0f, 0.0f);   ///< Color of particles at the end of their life.
        std::vector<Particle> cpuParticles; ///< Internal list of active CPU particles.
        std::vector<ParticleGPUData> activeParticles; ///< GPU data buffer built every frame for rendering.
    };

}
