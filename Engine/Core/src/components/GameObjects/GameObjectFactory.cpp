#include "components/GameObjects/GameObjectFactory.hpp"
#include <algorithm>

namespace vex {

    GameObjectFactory& GameObjectFactory::getInstance() {
        static GameObjectFactory instance;
        return instance;
    }

    void GameObjectFactory::registerCreator(const std::string& name, ObjectConstructor ctor, bool isDynamic) {
        creators[name] = [ctor, name](Engine& e, const std::string& instanceName) -> GameObject* {
            GameObject* obj = ctor(e, instanceName);
            obj->setObjectType(name);
            return obj;
        };

        allTypes.push_back(name);

        if (isDynamic) {
            dynamicTypes.push_back(name);
        } else {
            nonDynamicTypes.push_back(name);
        }
    }

    GameObject* GameObjectFactory::create(const std::string& type, Engine& engine, const std::string& name) {
        auto it = creators.find(type);
        if (it != creators.end()) {
            return it->second(engine, name);
        }
        log("Error: Type '%s' not registered", type.c_str());
        return nullptr;
    }

    std::vector<std::string> GameObjectFactory::GetAllRegisteredObjectTypes() {
        return allTypes;
    }

    std::vector<std::string> GameObjectFactory::GetNonDynamicRegisteredObjectTypes() {
        return nonDynamicTypes;
    }

    std::vector<std::string> GameObjectFactory::GetDynamicRegisteredObjectTypes() {
        return dynamicTypes;
    }

    void GameObjectFactory::clearDynamicGameObjects() {
        if(dynamicTypes.empty()) return;

        log("Hot Reload: Clearing %zu dynamic GameObjects...", dynamicTypes.size());
        for(const auto& name : dynamicTypes) {
            creators.erase(name);
        }
        dynamicTypes.clear();
        allTypes = nonDynamicTypes;
    }

    void GameObjectFactory::UnregisterGameObjects(){
        creators.clear();
        dynamicTypes.clear();
        nonDynamicTypes.clear();
        allTypes.clear();
    }
}
