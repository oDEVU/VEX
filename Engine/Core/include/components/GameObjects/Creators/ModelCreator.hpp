/**
 *  @file   ModelCreator.hpp
 *  @brief  This file defines functions for creating model objects and mesh components.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <string>
#include "components/GameObjects/ModelObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

namespace vex {
    /// @brief Creates model object From mesh and tranform components.
    /// @details Important: MeshComponent can be created normally by just creating meshComponent object and setting meshData.path, but using this function ensures mesh is loaded from file when called and not when rendered, ensuring no stutters at runtime.
    /// @param const std::string& name Name of the model object.
    /// @param MeshComponent meshComponent Mesh component of the model object.
    /// @param TransformComponent transformComponent Transform component of the model object.
    /// @param Engine& engine Reference to the engine.
    /// @param entt::entity parent Parent entity of the model object.
    /// @return Pointer to the created model object.
    ModelObject* createModelFromComponents(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, Engine& engine, entt::entity parent = entt::null);

    /// @brief Just creates meshcomponent from file. Copies meshData and texture paths for later backend specific processing.
    /// @param const std::string& path Path to the mesh file.
    /// @param Engine& engine Reference to the engine.
    /// @return MeshComponent created from the file.
    MeshComponent createMeshFromPath(const std::string& path, Engine& engine);

    /// @brief Creates model object from file. Esentially just runs two other functions just with empty transform component.
    /// @param const std::string& path Path to the mesh file.
    /// @param const std::string& name Name of the model object.
    /// @param Engine& engine Reference to the engine.
    /// @param entt::entity parent Parent entity of the model object.
    /// @return Pointer to the created model object.
    ModelObject* createModelFromPath(const std::string& path, const std::string& name, Engine& engine, entt::entity parent = entt::null);
}
