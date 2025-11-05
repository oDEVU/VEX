/**
 *  @file   Scene.hpp
 *  @brief  This file defines SceneManager class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

#include "Engine.hpp"
#include "entt/entity/fwd.hpp"

namespace vex {

/// @brief Scene class implements scene functionality like loading and holding game objects.
class Scene {
public:
/// @brief Constructor loads a scene from a file.
/// @param std::string& path - Path to the scene file.
/// @param Engine& engine - Reference to the engine instance.
Scene(const std::string& path, Engine& engine);

/// @brief Destructor clears the current scene.
~Scene();

/// @brief Function calls beginGame in all objects it holds. This function needs to be called in your projects beginGame method.
void sceneBegin();
/// @brief Function calls update in all objects it holds. This function needs to be called in your projects update method.
/// @param float deltaTime - Delta time since last frame.
void sceneUpdate(float deltaTime);

/// @brief Function to get game object by its unique name.
/// @param const std::string& name - Name of the game object to get.
/// @return std::shared_ptr<GameObject> - Shared pointer to the game object.
std::shared_ptr<GameObject> GetGameObjectByName(const std::string& name);

/// @brief Function to get game object by its entity.
/// @param entt::entity& entity - Entity of the game object to get.
/// @return std::shared_ptr<GameObject> - Shared pointer to the game object.
std::shared_ptr<GameObject> GetGameObjectByEntity(entt::entity& entity);

private:
std::vector<std::shared_ptr<GameObject>> m_objects;
};

} // namespace vex
