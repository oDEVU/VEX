#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vex {
    struct Transform {
        glm::vec3 position = {0.0f, 0.0f, 0.0f};
        glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 scale = {1.0f, 1.0f, 1.0f};

        glm::mat4 matrix() const {
            glm::mat4 mat(1.0f);
            mat = glm::translate(mat, position);
            mat = glm::rotate(mat, glm::radians(rotation.x), {1, 0, 0});
            mat = glm::rotate(mat, glm::radians(rotation.y), {0, 1, 0});
            mat = glm::rotate(mat, glm::radians(rotation.z), {0, 0, 1});
            mat = glm::scale(mat, scale);
            return mat;
        }
    };
}
