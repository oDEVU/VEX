#pragma once

#include <string>
#include "components/GameObjects/ModelObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

namespace vex {
    // Creates model object with empty transform component and meshcomponent from file
    ModelObject* createModelFromComponents(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, Engine& engine, entt::entity parent = entt::null);

    // Just creates meshcomponent from file
    MeshComponent createMeshFromPath(const std::string& path, Engine& engine);

    // Creates model from components
    ModelObject* createModelFromPath(const std::string& path, const std::string& name, Engine& engine, entt::entity parent = entt::null);
}
