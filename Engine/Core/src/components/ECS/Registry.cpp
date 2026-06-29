#include "components/ECS/Registry.hpp"
#include <unordered_map>
#include <string>

namespace vex {

uint32_t ComponentType::get_id_by_name(const char* name) {
    static std::unordered_map<std::string, uint32_t> type_map;
    static uint32_t next_type_id = 0;

    std::string typeName(name);

    auto it = type_map.find(typeName);
    if (it != type_map.end()) {
        return it->second;
    }

    uint32_t id = next_type_id++;
    type_map[typeName] = id;
    return id;
}

}
