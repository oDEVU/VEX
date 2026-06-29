/**
 * @file   AudioSourceComponent.hpp
 * @brief  Defines the AudioSourceComponent struct.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <SDL3/SDL.h>
#include "SDL3/SDL_audio.h"
#include "components/ErrorUtils.hpp"
#include "components/AssetTypes.hpp"
#include <string>
#include "components/VirtualFileSystem.hpp"

namespace vex {

    struct AudioClip {
        Uint8* buffer = nullptr;
        Uint32 length = 0;
        SDL_AudioSpec spec;
        bool valid = false;
        bool isVorbisAllocated = false;

        AudioClip(const std::string& path, vex::VirtualFileSystem* vfs);

        ~AudioClip() {
            if (buffer) {
                if (isVorbisAllocated) {
                    free(buffer);
                } else {
                    SDL_free(buffer);
                }
            }
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
        //if (clip) delete clip;
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
