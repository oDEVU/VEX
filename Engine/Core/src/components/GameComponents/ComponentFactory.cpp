#include "components/GameComponents/ComponentFactory.hpp"
#include "components/errorUtils.hpp"

namespace vex {

    ComponentRegistry& ComponentRegistry::getInstance() {
        static ComponentRegistry instance;
        return instance;
    }

    void ComponentRegistry::unregisterComponent(const std::string& name) {
            loaders.erase(name);
            savers.erase(name);
            inspectors.erase(name);
            checkers.erase(name);
            creators.erase(name);

            auto it = std::remove(registeredNames.begin(), registeredNames.end(), name);
            if (it != registeredNames.end()) {
                registeredNames.erase(it, registeredNames.end());
            }

            log("Component '%s' unregistered", name.c_str());
        }

        void ComponentRegistry::clearDynamicComponents() {
                if (dynamicComponents.empty()) return;

                log("Hot Reload: Clearing %zu game components...", dynamicComponents.size());

                for (const auto& name : dynamicComponents) {
                    // Manual unregister logic (copy of unregisterComponent but internal)
                    loaders.erase(name);
                    savers.erase(name);
                    inspectors.erase(name);
                    checkers.erase(name);
                    creators.erase(name);

                    auto it = std::remove(registeredNames.begin(), registeredNames.end(), name);
                    if (it != registeredNames.end()) {
                        registeredNames.erase(it, registeredNames.end());
                    }
                }

                dynamicComponents.clear();
            }

    bool ComponentRegistry::loadComponent(GameObject& obj, const std::string& type, const nlohmann::json& json) {
        auto it = loaders.find(type);
        if (it != loaders.end()) {
            it->second(obj, json);
            return true;
        }
        log("Error: Component type '%s' not registered", type.c_str());
        return false;
    }

    nlohmann::json ComponentRegistry::saveComponent(GameObject& obj, const std::string& type) {
        if (savers.find(type) != savers.end()) return savers[type](obj);
        return nullptr;
    }

    void ComponentRegistry::drawInspectorForObject(GameObject& obj) {
        for (auto& [name, inspector] : inspectors) {
            inspector(obj);
        }
    }

    std::vector<std::string> ComponentRegistry::getMissingComponents(const GameObject& obj) {
        std::vector<std::string> missing;
        for (const auto& name : registeredNames) {
            if (checkers.find(name) != checkers.end() && !checkers[name](obj)) {
                missing.push_back(name);
            }
        }
        return missing;
    }

    void ComponentRegistry::createComponent(GameObject& obj, const std::string& name) {
        if (creators.find(name) != creators.end()) {
            creators[name](obj);
        } else {
            log("Error: Cannot create component '%s', not registered.", name.c_str());
        }
    }

    const std::unordered_map<std::string, ComponentRegistry::ComponentInspector>& ComponentRegistry::getInspectors() const {
        return inspectors;
    }

    const std::vector<std::string>& ComponentRegistry::getRegisteredNames() const {
        return registeredNames;
    }

}
