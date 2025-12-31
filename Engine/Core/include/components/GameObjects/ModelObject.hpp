/**
 *  @file   ModelObject.hpp
 *  @brief  This file defines in engine ModelObject class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/errorUtils.hpp"


namespace vex {
    ///@brief ModelObject class represents a model object in the engine. Thanks to engine separation from the backend, its constructor takes more parameters than its allowed by GameObjectFactory, thus it can only be created by Model Creators methods.
    class ModelObject : public GameObject {
        public:
        /// @brief Constructor for ModelObject class. As said earlier, ModelObject can only be created by Model Creators methods. So if you are reading this you are doing something wrong.
            ModelObject(Engine& engine, const std::string& name, MeshComponent meshComponent = MeshComponent{}, TransformComponent transformComponent = TransformComponent{})
                : GameObject(engine, name) {
                AddComponent(TransformComponent{transformComponent});
                AddComponent(MeshComponent{meshComponent});
            }
            /// @brief Destructor for ModelObject class.
            ~ModelObject() override {
                log("Model destructor called");
            }
    };
}
