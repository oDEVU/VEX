#pragma once
  #include "GameObject.hpp"
  #include "components/GameComponents/BasicComponents.hpp"

  namespace vex {
  class ModelObject : public GameObject {
  public:
      ModelObject(Engine& engine, const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent)
          : GameObject(engine, name) {
          AddComponent(TransformComponent{transformComponent});
          AddComponent(MeshComponent{meshComponent});
          //m_engine.loadModel(modelPath, modelName, entity);
      }
      //void SetParameters(const nlohmann::json& params) override {
      //    if (params.contains("position")) GetComponent<TransformComponent>().position = params["position"];
      //    if (params.contains("rotation")) GetComponent<TransformComponent>().rotation = params["rotation"];
      //    if (params.contains("scale")) GetComponent<TransformComponent>().scale = params["scale"];
      //}
  };
  }
