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
#include <entt/entt.hpp>

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
     * @param entt::registry& reg - Reference to the main EnTT registry used for component lookups.
     */
    AudioSystem(entt::registry& reg);

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
     * @param entt::entity cameraEntity - The entity representing the listener (usually the active camera) for 3D calculations.
     */
    void Update(entt::entity cameraEntity);

    /**
     * @brief Shuts down the audio system.
     * @details Destroys all active audio streams, clears the clip cache, and closes the SDL audio device.
     */
    void Shutdown();

private:
    SDL_AudioDeviceID deviceID;
    VirtualFileSystem* vfs = nullptr;
    entt::registry& registry;

    /// @brief Cache of loaded audio clips to prevent reloading the same file multiple times. Key is the file path.
    std::unordered_map<std::string, std::unique_ptr<AudioClip>> clipCache;

    /// @brief Map linking entities to their active SDL audio streams.
    std::unordered_map<entt::entity, SDL_AudioStream*> streamMap;

    /**
     * @brief Callback triggered when an AudioSourceComponent is destroyed.
     * @details Ensures the associated SDL audio stream is destroyed to prevent memory leaks or playing "ghost" audio.
     * @param entt::registry& registry - The registry where the destruction occurred.
     * @param entt::entity entity - The entity that was destroyed or had the component removed.
     */
    void OnAudioComponentDestroyed(entt::registry& registry, entt::entity entity);
};

}
