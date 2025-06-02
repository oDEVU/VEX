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

        glm::vec3 getForwardVector() const {
            float pitch = glm::radians(rotation.x);
            float yaw = glm::radians(rotation.y);

            return glm::normalize(glm::vec3(
                cos(yaw) * cos(pitch),
                sin(pitch),
                sin(yaw) * cos(pitch)
            ));
        }

        glm::vec3 getRightVector() {
            glm::vec3 forward = getForwardVector();
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            return glm::normalize(glm::cross(forward, worldUp));
        }

        glm::vec3 getUpVector() {
            glm::vec3 forward = getForwardVector();
            glm::vec3 right = getRightVector();
            return glm::normalize(glm::cross(right, forward));
        }
    };
}
