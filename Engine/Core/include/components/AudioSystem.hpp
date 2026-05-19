/**
 * @file   AudioSystem.hpp
 * @brief  Audio system for managing audio sources, playback, and 3D spatialization.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <unordered_map>
#include <memory>
#include <string>

#include <SDL3/SDL.h>
#include "components/ECS/ECS.hpp"

#include "components/VirtualFileSystem.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/AudioSourceComponent.hpp"

namespace vex {

/**
 * @brief System responsible for managing audio playback, mixing, and spatialization.
 * @details Handles the lifecycle of SDL audio streams, loads audio clips via the VirtualFileSystem,
 * and updates audio properties (volume, pitch, 3D position) every frame.
 */
class AudioSystem {
public:
    /**
     * @brief Constructor for AudioSystem.
     * @param vex::Registry& reg - Reference to the main EnTT registry used for component lookups.
     */
    AudioSystem(vex::Registry& reg);

    /**
     * @brief Initializes the audio subsystem and binding to the VFS.
     * @details Sets up the SDL audio subsystem, opens the default playback device, and connects
     * the component destruction signal to handle automatic stream cleanup.
     * @param vex::VirtualFileSystem* vfs - Pointer to the initialized VirtualFileSystem for loading audio assets.
     */
    void Init(vex::VirtualFileSystem* vfs);

    /**
     * @brief Main update loop for the audio system.
     * @details Iterates over all entities with AudioSourceComponents. It handles:
     * - Auto-loading audio clips if the path is set but the clip is null.
     * - Processing state changes (Play/Stop/Pause).
     * - Refilling audio buffers for looping sounds.
     * - Calculating 3D spatial audio volume based on distance to the camera/listener.
     * @param vex::Entity cameraEntity - The entity representing the listener (usually the active camera) for 3D calculations.
     */
    void Update(vex::Entity cameraEntity);

    /**
     * @brief Shuts down the audio system.
     * @details Destroys all active audio streams, clears the clip cache, and closes the SDL audio device.
     */
    void Shutdown();

    /**
     * @brief Quickly plays a 2D sound without requiring an entity or AudioSourceComponent.
     * @details Creates Standalone stream for the sound, plays it, and automatically cleans up when playback completes.
     * @param const std::string& filePath - Path to the audio file to play.
     * @param float volume - Volume level (default 1.0f). Range: 0.0 - 1.0+
     * @param float pitch - Pitch/speed multiplier (default 1.0f). Range: > 0.0
     * @param bool loop - Whether the sound should loop (default false).
     */
    void PlaySound2D(const std::string& filePath, float volume = 1.0f, float pitch = 1.0f, bool loop = false);

private:
    SDL_AudioDeviceID deviceID;
    VirtualFileSystem* vfs = nullptr;
    vex::Registry& registry;

    /// @brief Cache of loaded audio clips to prevent reloading the same file multiple times. Key is the file path.
    std::unordered_map<std::string, std::unique_ptr<AudioClip>> clipCache;

    /// @brief Map linking entities to their active SDL audio streams.
    std::unordered_map<vex::Entity, SDL_AudioStream*> streamMap;

    /**
     * @brief Callback triggered when an AudioSourceComponent is destroyed.
     * @details Ensures the associated SDL audio stream is destroyed to prevent memory leaks or playing "ghost" audio.
     * @param vex::Registry& registry - The registry where the destruction occurred.
     * @param vex::Entity entity - The entity that was destroyed or had the component removed.
     */
    void OnAudioComponentDestroyed(vex::Registry& registry, vex::Entity entity);

    struct StandaloneAudio {
        SDL_AudioStream* stream;
        AudioClip* clip;
        bool loop;
    };
    std::vector<StandaloneAudio> standaloneStreams;
};

}
