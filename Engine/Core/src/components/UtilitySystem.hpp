#include "components/GameComponents/ParticleEmitterComponent.hpp"
#include "components/GameComponents/UtilityComponents.hpp"
#include <cmath>

namespace vex {

/// @brief Processes CPU-side particle simulation and prepares GPU data.
/// @details Spawns new particles based on spawn rate, updates their physics (gravity, velocity),
/// scales and colors over their lifetime, and removes dead particles. Maps the remaining active
/// particles to a structure suitable for the GPU (ParticleGPUData).
/// @param vex::Registry& registry - The ECS registry to fetch emitters and transforms.
/// @param float deltaTime - Time elapsed since the last frame.
void ProcessParticles(vex::Registry& registry, float deltaTime) {
    vex::View<TransformComponent, ParticleEmitterComponent> particleView(registry);
    particleView.each([&](vex::Entity entity, TransformComponent& trans, ParticleEmitterComponent& emit) {
        trans.recalculateMatrix();

        emit.spawnTimer += deltaTime;
        static int frame = 0;
        if (frame++ % 60 == 0) vex::log(LogLevel::INFO, "ProcessParticles running... CPU count: %d, spawnTimer: %f", (int)emit.cpuParticles.size(), emit.spawnTimer);
        if (emit.spawnRate <= 0.001f) emit.spawnRate = 0.001f;
        while (emit.spawnTimer >= emit.spawnRate) {
            emit.spawnTimer -= emit.spawnRate;
            if(!emit.active) break;
            Particle p;
            p.position = trans.getWorldPosition();

            float rx = ((rand() % 200) / 100.0f) - 1.0f;
            float ry = ((rand() % 200) / 100.0f) - 1.0f;
            float rz = ((rand() % 200) / 100.0f) - 1.0f;

            p.velocity = emit.initialVelocity + (emit.velocityVariation * glm::vec3(rx, ry, rz));

            float rLife = ((rand() % 200) / 100.0f) - 1.0f;
            p.life = p.startingLife = emit.particleLife + (emit.particleLifeVariation * rLife);

            p.startSize = emit.startSize;
            p.endSize = emit.endSize;
            p.startColor = emit.startColor;
            p.endColor = emit.endColor;

            emit.cpuParticles.push_back(p);
        }

        emit.activeParticles.clear();
        for (auto it = emit.cpuParticles.begin(); it != emit.cpuParticles.end(); ) {
            it->life -= deltaTime;
            if (it->life <= 0.0f) {
                it = emit.cpuParticles.erase(it);
            } else {
                it->velocity += emit.gravity * deltaTime;
                it->position += it->velocity * deltaTime;

                float t = 1.0f - (it->life / it->startingLife);
                float currentSize = glm::mix(it->startSize, it->endSize, t);

                glm::vec4 sColor(it->startColor.r, it->startColor.g, it->startColor.b, it->startColor.a);
                glm::vec4 eColor(it->endColor.r, it->endColor.g, it->endColor.b, it->endColor.a);
                glm::vec4 currentColor = glm::mix(sColor, eColor, t);

                ParticleGPUData gpuData{};
                gpuData.position = glm::vec4(it->position, 1.0f);
                gpuData.scaleX = currentSize;
                gpuData.scaleY = currentSize;
                gpuData.color = currentColor;
                gpuData.isUnlit = emit.isUnlit ? 1 : 0;

                emit.activeParticles.push_back(gpuData);
                ++it;
            }
        }
    });
}


void ProcessUtilityComponents(vex::Registry& registry, float deltaTime, Engine& engine) {
    vex::View<OscillatorComponent, TransformComponent> oscView(registry);
    oscView.each([&](vex::Entity entity, OscillatorComponent& osc, TransformComponent& transform) {
        if (!osc.initialized) {
            osc.startPosition = transform.getLocalPosition();
            osc.initialized = true;
        }

        osc.currentTime += deltaTime;
        float wave = std::sin((osc.currentTime + osc.timeOffset) * osc.frequency) * osc.amplitude;

        transform.setLocalPosition(osc.startPosition + (osc.axis * wave));
    });

    vex::View<TweenComponent, TransformComponent> tweenView(registry);
    tweenView.each([&](vex::Entity entity, TweenComponent& tween, TransformComponent& transform) {
        if (!tween.initialized) {
            tween.startPosition = transform.getLocalPosition();
            tween.initialized = true;
        }

        tween.timeElapsed += deltaTime;
        float t = std::clamp(tween.timeElapsed / tween.duration, 0.0f, 1.0f);

        float ease = t * t * (3.0f - 2.0f * t);

        glm::vec3 currentTarget = tween.returning ? tween.startPosition : (tween.startPosition + tween.targetLocalOffset);
        glm::vec3 currentStart = tween.returning ? (tween.startPosition + tween.targetLocalOffset) : tween.startPosition;

        transform.setLocalPosition(glm::mix(currentStart, currentTarget, ease));

        if (t >= 1.0f) {
            if (tween.pingPong) {
                tween.returning = !tween.returning;
                tween.timeElapsed = 0.0f;
            }
        }
    });

    vex::View<LifetimeComponent> lifetimeView(registry);
    lifetimeView.each([&](vex::Entity entity, LifetimeComponent& lifetime) {
        lifetime.timeElapsed += deltaTime;

        if (lifetime.timeElapsed >= lifetime.lifespan) {
            for (const auto& sceneName : engine.getSceneManager()->GetAllSceneNames()) {
                Scene* scene = engine.getSceneManager()->GetScene(sceneName);
                if (scene) {
                    GameObject* obj = scene->GetGameObjectByEntity(entity);
                    if (obj) {
                        scene->DestroyGameObject(obj);
                        break;
                    }
                }
            }
        }
    });
}

} // namespace vex