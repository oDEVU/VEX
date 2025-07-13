#pragma once
#include "GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/errorUtils.hpp"

namespace vex {
    class ModelObject : public GameObject {
        public:
            ModelObject(Engine& engine, const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent)
                : GameObject(engine, name) {
                AddComponent(TransformComponent{transformComponent});
                AddComponent(MeshComponent{meshComponent});
            }

            ~ModelObject() override {
                cleanup(GetComponent<NameComponent>().name,GetComponent<MeshComponent>());
                GetComponent<MeshComponent>().meshData.clear();
                log("Model destructor called");
            }
            //void SetParameters(const nlohmann::json& params) override {
            //if (params.contains("position")) GetComponent<TransformComponent>().position = params["position"];
            //if (params.contains("rotation")) GetComponent<TransformComponent>().rotation = params["rotation"];
            //if (params.contains("scale")) GetComponent<TransformComponent>().scale = params["scale"];
            //}
            std::function<void(std::string&, MeshComponent)> cleanup;
    };
}
