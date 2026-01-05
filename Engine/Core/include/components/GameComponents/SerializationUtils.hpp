/**
 * @file   SerializationUtils.hpp
 * @brief  Allows for imgui and json serialization of components, for easy loading, saving and drawing them.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glm {
    // Serialize vec3
    inline void to_json(nlohmann::json& j, const vec3& v) {
        j = nlohmann::json::array({v.x, v.y, v.z});
    }

    // Deserialize vec3
    inline void from_json(const nlohmann::json& j, vec3& v) {
        if(j.is_array() && j.size() >= 3) {
            v.x = j[0]; v.y = j[1]; v.z = j[2];
        }
    }

    // Serialize vec4
    inline void to_json(nlohmann::json& j, const vec4& v) {
        j = nlohmann::json::array({v.x, v.y, v.z, v.w});
    }

    // Deserialize vec4
    inline void from_json(const nlohmann::json& j, vec4& v) {
        if(j.is_array() && j.size() >= 4) {
            v.x = j[0]; v.y = j[1]; v.z = j[2]; v.w = j[3];
        }
    }

    // Serialize quat
    inline void to_json(nlohmann::json& j, const quat& q) {
         j = nlohmann::json::array({q.w, q.x, q.y, q.z});
    }

    inline void from_json(const nlohmann::json& j, quat& q) {
        if(j.is_array() && j.size() >= 4) {
            q.w = j[0]; q.x = j[1]; q.y = j[2]; q.z = j[3];
        }
    }
}
