#include "components/AudioSystem.hpp"
#include "components/errorUtils.hpp"

namespace vex {

AudioSystem::AudioSystem(entt::registry& reg) : registry(reg) {
}

void AudioSystem::Init(vex::VirtualFileSystem* vfs) {
    this->vfs = vfs;

    registry.on_destroy<AudioSourceComponent>().connect<&AudioSystem::OnAudioComponentDestroyed>(this);

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        throw_error("Failed to initialize audio subsystem");
    }

    deviceID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
}

void AudioSystem::Update(entt::entity cameraEntity) {
    auto view = registry.view<AudioSourceComponent>();

    glm::vec3 listenerPos = glm::vec3(0.0f);
    if (registry.valid(cameraEntity) && registry.all_of<TransformComponent>(cameraEntity)) {
        listenerPos = registry.get<TransformComponent>(cameraEntity).getWorldPosition();
    }else{
        log(LogLevel::ERROR, "No valid camera entity found");
    }

    for (auto entity : view) {
        auto& comp = view.get<AudioSourceComponent>(entity);

        if (comp.autoPlay) {
            comp.Play();
            comp.autoPlay = false;
        }

        if (comp.getAudioClip() == nullptr && !comp.audioFilePath.empty()) {
            if (clipCache.find(comp.audioFilePath) == clipCache.end()) {
                auto newClip = std::make_unique<AudioClip>(comp.audioFilePath, vfs);
                if (newClip->valid) {
                    clipCache[comp.audioFilePath] = std::move(newClip);
                } else {
                    comp.Stop();
                    comp.stateDirty = false;
                    continue;
                }
            }
            comp.setAudioClip(clipCache[comp.audioFilePath].get());
        }

        if (comp.stateDirty) {
            SDL_AudioStream* stream = streamMap[entity];

            if (comp.getState() == AudioState::PLAYING) {
                if (comp.getAudioClip() && comp.getAudioClip()->valid) {
                    if (stream) SDL_DestroyAudioStream(stream);

                    stream = SDL_CreateAudioStream(&comp.getAudioClip()->spec, NULL);
                    SDL_BindAudioStream(deviceID, stream);
                    SDL_PutAudioStreamData(stream, comp.getAudioClip()->buffer, comp.getAudioClip()->length);

                    streamMap[entity] = stream;
                }
            }
            else if (comp.getState() == AudioState::STOPPED) {
                if (stream) {
                    SDL_DestroyAudioStream(stream);
                    streamMap.erase(entity);
                    stream = nullptr;
                }
            }
            else if (comp.getState() == AudioState::PAUSED) {
                if (stream) SDL_UnbindAudioStream(stream);
            }

            comp.stateDirty = false;
        }

        if (streamMap.find(entity) == streamMap.end()) continue;
        SDL_AudioStream* stream = streamMap[entity];

        if (!stream) continue;

        if (comp.loop && SDL_GetAudioStreamAvailable(stream) < comp.getAudioClip()->length / 2) {
            SDL_PutAudioStreamData(stream, comp.getAudioClip()->buffer, comp.getAudioClip()->length);
        }

        if (!comp.loop && SDL_GetAudioStreamAvailable(stream) == 0) {
            SDL_DestroyAudioStream(stream);
            streamMap.erase(entity);
            comp.Stop();
            comp.stateDirty = false;
            continue;
        }

        float finalVolume = comp.volume;
        if (comp.is3D) {
            if (registry.all_of<TransformComponent>(entity)) {
                auto& transform = registry.get<TransformComponent>(entity);
                float dist = glm::distance(listenerPos, transform.getWorldPosition());

                if (dist > comp.distance) {
                    finalVolume = 0.0f;
                } else {
                    finalVolume = comp.volume * (1.0f - (dist / comp.distance));
                }
            }
        }

        SDL_SetAudioStreamGain(stream, finalVolume);
        SDL_SetAudioStreamFrequencyRatio(stream, comp.pitch);
    }
}

void AudioSystem::Shutdown() {
    for (auto& [entity, stream] : streamMap) {
        SDL_DestroyAudioStream(stream);
    }
    streamMap.clear();
    clipCache.clear();
    SDL_CloseAudioDevice(deviceID);
}

void AudioSystem::OnAudioComponentDestroyed(entt::registry& registry, entt::entity entity) {
    auto it = streamMap.find(entity);
    if (it != streamMap.end()) {
        SDL_DestroyAudioStream(it->second);
        streamMap.erase(it);
    }
}

}
