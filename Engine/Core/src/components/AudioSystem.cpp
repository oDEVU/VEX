#include "components/AudioSystem.hpp"
#include "components/ErrorUtils.hpp"
#include "../../../thirdparty/stb/stb_vorbis.c"

namespace vex {

    AudioClip::AudioClip(const std::string& path, vex::VirtualFileSystem* vfs) {
        if (!vfs) return;

        auto fileData = vfs->load_file(path);
        if (!fileData) {
            vex::log(vex::LogLevel::ERROR, "AudioClip: VFS failed to load path: %s", path.c_str());
            return;
        }

        bool isOgg = (path.length() >= 4 && path.substr(path.length() - 4) == ".ogg");

        if (isOgg) {
            int channels, sampleRate;
            short* decodedData;

            int samplesPerChannel = stb_vorbis_decode_memory(
                reinterpret_cast<const unsigned char*>(fileData->data.data()),
                fileData->size,
                &channels,
                &sampleRate,
                &decodedData
            );

            if (samplesPerChannel >= 0) {
                SDL_zero(spec);
                spec.freq = sampleRate;
                spec.format = SDL_AUDIO_S16;
                spec.channels = channels;

                buffer = reinterpret_cast<Uint8*>(decodedData);
                length = samplesPerChannel * channels * sizeof(short);

                valid = true;
                isVorbisAllocated = true;
            } else {
                vex::log(vex::LogLevel::ERROR, "AudioClip: stb_vorbis failed to decode: %s", path.c_str());
            }
        }
        else {
            SDL_IOStream* io = SDL_IOFromConstMem(fileData->data.data(), fileData->size);
            if (SDL_LoadWAV_IO(io, true, &spec, &buffer, &length)) {
                valid = true;
            } else {
                vex::log(vex::LogLevel::ERROR, "AudioClip: SDL_LoadWAV failed for %s. SDL Error: %s", path.c_str(), SDL_GetError());
            }
        }
    }

AudioSystem::AudioSystem(vex::Registry& reg) : registry(reg) {
}

void AudioSystem::Init(vex::VirtualFileSystem* vfs) {
    this->vfs = vfs;

    registry.on_destroy<AudioSourceComponent>([this](vex::Entity entity, AudioSourceComponent&) {
        OnAudioComponentDestroyed(registry, entity);
    });

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        throw_error("Failed to initialize audio subsystem");
    }

    deviceID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
}

void AudioSystem::Update(vex::Entity cameraEntity) {
    glm::vec3 listenerPos = glm::vec3(0.0f);
    if (registry.has<TransformComponent>(cameraEntity)) {
        listenerPos = registry.get<TransformComponent>(cameraEntity).getWorldPosition();
    }else{
        log(LogLevel::ERROR, "No valid camera entity found");
    }

    vex::View<AudioSourceComponent>(registry).each([&, this, listenerPos](vex::Entity entity, AudioSourceComponent& comp) {
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
                    return;
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

        if (streamMap.find(entity) == streamMap.end()) {
            return;
        }
        SDL_AudioStream* stream = streamMap[entity];

        if (!stream) {
            return;
        }

        if (comp.loop && SDL_GetAudioStreamAvailable(stream) < comp.getAudioClip()->length / 2) {
            SDL_PutAudioStreamData(stream, comp.getAudioClip()->buffer, comp.getAudioClip()->length);
        }

        if (!comp.loop && SDL_GetAudioStreamAvailable(stream) == 0) {
            SDL_DestroyAudioStream(stream);
            streamMap.erase(entity);
            comp.Stop();
            comp.stateDirty = false;
            return;
        }

        float finalVolume = comp.volume;
        if (comp.is3D) {
            if (registry.has<TransformComponent>(entity)) {
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
    });

    for (auto it = standaloneStreams.begin(); it != standaloneStreams.end(); ) {
        SDL_AudioStream* stream = it->stream;

        if (it->loop && SDL_GetAudioStreamAvailable(stream) < it->clip->length / 2) {
            SDL_PutAudioStreamData(stream, it->clip->buffer, it->clip->length);
            ++it;
        }
        else if (!it->loop && SDL_GetAudioStreamAvailable(stream) == 0) {
            SDL_DestroyAudioStream(stream);
            it = standaloneStreams.erase(it);
        }
        else {
            ++it;
        }
    }
}

void AudioSystem::Shutdown() {
    for (auto& sa : standaloneStreams) {
        SDL_DestroyAudioStream(sa.stream);
    }
    standaloneStreams.clear();

    for (auto& [entity, stream] : streamMap) {
        SDL_DestroyAudioStream(stream);
    }
    streamMap.clear();
    clipCache.clear();
    SDL_CloseAudioDevice(deviceID);
}

void AudioSystem::OnAudioComponentDestroyed(vex::Registry& registry, vex::Entity entity) {
    auto it = streamMap.find(entity);
    if (it != streamMap.end()) {
        SDL_DestroyAudioStream(it->second);
        streamMap.erase(it);
    }
}

void AudioSystem::PlaySound2D(const std::string& filePath, float volume, float pitch, bool loop) {
    try{
    if (clipCache.find(filePath) == clipCache.end()) {
        auto newClip = std::make_unique<AudioClip>(filePath, vfs);
        if (newClip->valid) {
            clipCache[filePath] = std::move(newClip);
        } else {
            log(LogLevel::ERROR, "PlaySound2D: Failed to load audio clip - %s", filePath.c_str());
            return;
        }
    }

    AudioClip* clip = clipCache[filePath].get();

    SDL_AudioStream* stream = SDL_CreateAudioStream(&clip->spec, NULL);
    if (!stream) return;

    SDL_SetAudioStreamGain(stream, volume);
    SDL_SetAudioStreamFrequencyRatio(stream, pitch);

    SDL_BindAudioStream(deviceID, stream);
    SDL_PutAudioStreamData(stream, clip->buffer, clip->length);

    standaloneStreams.push_back({stream, clip, loop});
    } catch (const std::exception& e) {
        log(LogLevel::ERROR, "PlaySound2D: Failed to play sound - %s", e.what());
    }
}

}
