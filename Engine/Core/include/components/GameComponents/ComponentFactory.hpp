#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/errorUtils.hpp"
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <string>

namespace vex {

class GameObject;

class ComponentRegistry {
public:
    using ComponentLoader = std::function<void(GameObject&, const nlohmann::json&)>;

    static ComponentRegistry& getInstance() {
        static ComponentRegistry instance;
        return instance;
    }

    void registerComponent(const std::string& type, ComponentLoader loader) {
        loaders[type] = loader;
    }

    bool loadComponent(GameObject& obj, const std::string& type, const nlohmann::json& json) {
        auto it = loaders.find(type);
        if (it != loaders.end()) {
            it->second(obj, json);
            return true;
        }
        log("Error: Component type '%s' not registered", type.c_str());
        return false;
    }

private:
    ComponentRegistry() = default;
    std::unordered_map<std::string, ComponentLoader> loaders;
};

#define REGISTER_COMPONENT(ComponentType, LoadFunction) \
    namespace { \
        struct ComponentType##Registrar { \
            ComponentType##Registrar() { \
                ComponentRegistry::getInstance().registerComponent(#ComponentType, LoadFunction); \
            } \
        }; \
        static ComponentType##Registrar ComponentType##registrar; \
    }
}
