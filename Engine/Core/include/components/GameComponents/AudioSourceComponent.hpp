/**
 * @file   AudioSourceComponent.hpp
 * @brief  Defines the AudioSourceComponent struct.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <SDL3/SDL.h>
#include "SDL3/SDL_audio.h"
#include "components/errorUtils.hpp"
#include "components/assetTypes.hpp"
#include <string>
#include "components/VirtualFileSystem.hpp"

namespace vex {

struct AudioClip {
    Uint8* buffer = nullptr;
    Uint32 length = 0;
    SDL_AudioSpec spec;
    bool valid = false;

    AudioClip(const std::string& path, vex::VirtualFileSystem* vfs) {
        if (!vfs) return;

        auto fileData = vfs->load_file(path);
        if (!fileData) {
            vex::log(vex::LogLevel::ERROR, "AudioClip: VFS failed to load path: %s", path.c_str());
            return;
        }

        SDL_IOStream* io = SDL_IOFromConstMem(fileData->data.data(), fileData->size);
        if (SDL_LoadWAV_IO(io, true, &spec, &buffer, &length)) {
            valid = true;
        } else {
            vex::log(vex::LogLevel::ERROR, "AudioClip: SDL_LoadWAV failed for %s. SDL Error: %s", path.c_str(), SDL_GetError());
        }
    }

    ~AudioClip() {
        if (buffer) SDL_free(buffer);
    }
};

enum class AudioState {
    STOPPED,
    PLAYING,
    PAUSED
};

struct AudioSourceComponent {
private:
    AudioClip* clip = nullptr;
    AudioState state = AudioState::STOPPED;
public:
    audio_asset_path audioFilePath;
    bool loop = false;
    bool is3D = false;
    bool autoPlay = false;
    float volume = 1.0f;
    float pitch = 1.0f;
    float distance = 50.0f;

    bool stateDirty = false;

    void setAudioClip(AudioClip* newClip) {
        if (clip) delete clip;
        clip = newClip;
    }

    AudioClip* getAudioClip() {
        return clip;
    }

    void Play() {
        if (state != AudioState::PLAYING) {
            state = AudioState::PLAYING;
            stateDirty = true;
        }
    }

    void Stop() {
        if (state != AudioState::STOPPED) {
            state = AudioState::STOPPED;
            stateDirty = true;
        }
    }

    void Pause() {
        if (state != AudioState::PAUSED) {
            state = AudioState::PAUSED;
            stateDirty = true;
        }
    }

    AudioState getState() {
        return state;
    }
};

}
