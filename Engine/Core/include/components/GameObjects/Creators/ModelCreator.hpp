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
    /// @details Important: MeshComponent cannot be just created by user you need to use createMeshFromPath function to copy meshData from file to Component.
    ModelObject* createModelFromComponents(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, Engine& engine, entt::entity parent = entt::null);

    /// @brief Just creates meshcomponent from file. Copies meshData and texture paths for later backend specific processing.
    MeshComponent createMeshFromPath(const std::string& path, Engine& engine);

    /// @brief Creates model object from file. Esentially just runs two other functions just with empty transform component.
    ModelObject* createModelFromPath(const std::string& path, const std::string& name, Engine& engine, entt::entity parent = entt::null);
}
