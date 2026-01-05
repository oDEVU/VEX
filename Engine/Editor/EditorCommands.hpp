/**
 * @file   EditorCommands.hpp
 * @brief  Command pattern implementation for Editor Undo/Redo system.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include "components/GameObjects/GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/ComponentFactory.hpp"
#include "components/SceneManager.hpp"
#include <vector>
#include <deque>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace vex {

    /// @brief Base Command Interface
    struct ICommand {
        virtual ~ICommand() = default;
        virtual void Execute() = 0;
        virtual void Undo() = 0;
    };

    /// @brief Gizmo Movement Command
    class TransformCommand : public ICommand {
        vex::GameObject* m_object;
        glm::vec3 m_oldPos, m_oldScale, m_oldRot;
        glm::vec3 m_newPos, m_newScale, m_newRot;
        bool m_valid;

    public:
    /// @brief Constructor, saves old and new state.
        TransformCommand(vex::GameObject* obj,
                         glm::vec3 oldP, glm::vec3 oldR, glm::vec3 oldS,
                         glm::vec3 newP, glm::vec3 newR, glm::vec3 newS)
            : m_object(obj),
              m_oldPos(oldP), m_oldRot(oldR), m_oldScale(oldS),
              m_newPos(newP), m_newRot(newR), m_newScale(newS)
        {
            m_valid = (obj != nullptr);
        }

        /// @brief Does the transform changes.
        void Execute() override {
            if (!m_valid || !m_object) return;
            auto& tc = m_object->GetComponent<vex::TransformComponent>();
            tc.setLocalPosition(m_newPos);
            tc.rotation = m_newRot;
            tc.setLocalScale(m_newScale);
            tc.convertRot();
            tc.enableLastTransformed();
        }

        /// @brief Undoes the transfom changes.
        void Undo() override {
            if (!m_valid || !m_object) return;
            auto& tc = m_object->GetComponent<vex::TransformComponent>();
            tc.setLocalPosition(m_oldPos);
            tc.rotation = m_oldRot;
            tc.setLocalScale(m_oldScale);
            tc.convertRot();
            tc.enableLastTransformed();
        }
    };

    /// @brief Serialized object data for recreation on undo
    struct SerializedObject {
        std::string name;
        std::string type;
        entt::entity originalID;
        entt::entity originalParentID;
        std::unordered_map<std::string, nlohmann::json> components;
    };

    /// @brief Saves Delete action command
    class DeleteCommand : public ICommand {
        vex::Engine& m_engine;
        std::string m_sceneName;

        std::vector<SerializedObject> m_serializedData;

        std::vector<entt::entity> m_runtimeIDs;

        /// @brief Gather scene hierarhy for later rebuild.
        void GatherHierarchy(vex::GameObject* obj, std::vector<vex::GameObject*>& outList) {
            if (!obj) return;
            outList.push_back(obj);

            auto& registry = m_engine.getRegistry();
            auto view = registry.view<vex::TransformComponent>();
            entt::entity parentID = obj->GetEntity();

            for (auto entity : view) {
                if (view.get<vex::TransformComponent>(entity).getParent() == parentID) {
                    auto* childObj = m_engine.getSceneManager()->GetScene(m_sceneName)->GetGameObjectByEntity(entity);
                    if (childObj) {
                        GatherHierarchy(childObj, outList);
                    }
                }
            }
        }

    public:
    /// @brief Serialized object data for deletion
        DeleteCommand(vex::Engine& engine, std::vector<vex::GameObject*> rootsToDelete)
            : m_engine(engine)
        {
            m_sceneName = engine.getSceneManager()->getLastSceneName();

            std::vector<vex::GameObject*> allObjects;
            for (auto* root : rootsToDelete) {
                GatherHierarchy(root, allObjects);
            }

            for (auto* obj : allObjects) {
                SerializedObject data;
                data.name = obj->GetComponent<vex::NameComponent>().name;
                data.type = obj->getObjectType();
                data.originalID = obj->GetEntity();

                if (obj->HasComponent<vex::TransformComponent>()) {
                    data.originalParentID = obj->GetComponent<vex::TransformComponent>().getParent();
                } else {
                    data.originalParentID = entt::null;
                }

                const auto& regNames = vex::ComponentRegistry::getInstance().getRegisteredNames();
                for (const auto& compName : regNames) {
                    nlohmann::json json = vex::ComponentRegistry::getInstance().saveComponent(*obj, compName);
                    if (!json.is_null()) {
                        data.components[compName] = json;
                    }
                }
                m_serializedData.push_back(data);
            }
        }

        /// @brief Executes the delete command.
        void Execute() override {
            if (m_runtimeIDs.empty()) return;

            auto* scene = m_engine.getSceneManager()->GetScene(m_sceneName);
            if (!scene) return;

            m_engine.WaitForGpu();

            for (entt::entity entity : m_runtimeIDs) {
                if (m_engine.getRegistry().valid(entity)) {
                    auto* obj = scene->GetGameObjectByEntity(entity);
                    if (obj) {
                        scene->DestroyGameObject(obj);
                    } else {
                        m_engine.getRegistry().destroy(entity);
                    }
                }
            }

            m_runtimeIDs.clear();
            m_engine.refreshForObject();
        }

        /// @brief Undos the delete command with saved/serialized data
        void Undo() override {
            auto* scene = m_engine.getSceneManager()->GetScene(m_sceneName);
            if (!scene) return;

            std::unordered_map<entt::entity, entt::entity> oldToNewIDMap;
            m_runtimeIDs.clear();

            for (const auto& data : m_serializedData) {
                vex::GameObject* newObj = vex::GameObjectFactory::getInstance().create(data.type, m_engine, data.name);
                if (!newObj) continue;

                for (const auto& [compName, jsonData] : data.components) {
                    vex::ComponentRegistry::getInstance().loadComponent(*newObj, compName, jsonData);
                }

                scene->AddEditorGameObject(newObj);

                entt::entity newID = newObj->GetEntity();
                oldToNewIDMap[data.originalID] = newID;
                m_runtimeIDs.push_back(newID);
            }

            auto& registry = m_engine.getRegistry();

            for (const auto& data : m_serializedData) {
                if (!oldToNewIDMap.contains(data.originalID)) continue;

                entt::entity newEntityID = oldToNewIDMap[data.originalID];

                if (registry.all_of<vex::TransformComponent>(newEntityID)) {
                    auto& tc = registry.get<vex::TransformComponent>(newEntityID);
                    entt::entity oldParent = data.originalParentID;

                    if (oldToNewIDMap.contains(oldParent)) {
                        tc.setParent(oldToNewIDMap[oldParent]);
                    } else {
                        tc.setParent(oldParent);
                    }
                }
            }

            m_engine.refreshForObject();
        }
    };
}
