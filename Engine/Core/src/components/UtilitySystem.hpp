#include "components/GameComponents/UtilityComponents.hpp"
#include <cmath>

namespace vex {

void ProcessUtilityComponents(entt::registry& registry, float deltaTime, Engine& engine) {
    auto oscView = registry.view<OscillatorComponent, TransformComponent>();
    for (auto entity : oscView) {
        auto& osc = oscView.get<OscillatorComponent>(entity);
        auto& transform = oscView.get<TransformComponent>(entity);

        if (!osc.initialized) {
            osc.startPosition = transform.getLocalPosition();
            osc.initialized = true;
        }

        osc.currentTime += deltaTime;
        float wave = std::sin((osc.currentTime + osc.timeOffset) * osc.frequency) * osc.amplitude;

        transform.setLocalPosition(osc.startPosition + (osc.axis * wave));
    }

    auto tweenView = registry.view<TweenComponent, TransformComponent>();
    for (auto entity : tweenView) {
        auto& tween = tweenView.get<TweenComponent>(entity);
        auto& transform = tweenView.get<TransformComponent>(entity);

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
    }

    auto lifetimeView = registry.view<LifetimeComponent>();
    for (auto entity : lifetimeView) {
        auto& lifetime = lifetimeView.get<LifetimeComponent>(entity);
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
    }
}

} // namespace vex
