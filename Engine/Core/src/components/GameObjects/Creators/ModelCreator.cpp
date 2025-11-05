#include <string>
#include "components/GameObjects/Creators/ModelCreator.hpp"

#include "Engine.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/backends/vulkan/Interface.hpp"
#include "components/backends/vulkan/MeshManager.hpp"

namespace vex {
    ModelObject* createModelFromComponents(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, Engine& engine, entt::entity parent){
        return engine.getInterface()->getMeshManager().createModel(name, meshComponent, transformComponent, engine, parent);
    }
    MeshComponent createMeshFromPath(const std::string& path, Engine& engine){
        return engine.getInterface()->getMeshManager().loadMesh(path);
    }
    ModelObject* createModelFromPath(const std::string& path, const std::string& name, Engine& engine, entt::entity parent){
        MeshComponent meshComponent = createMeshFromPath(path, engine);
        TransformComponent transformComponent(engine.getRegistry());
        return createModelFromComponents(name, meshComponent, transformComponent, engine, parent);
    }
}
