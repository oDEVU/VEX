#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} camera;

layout(set = 0, binding = 1) uniform ModelUBO {
    mat4 model;
} object;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;

void main() {
    gl_Position = camera.proj * camera.view * object.model * vec4(inPosition, 1.0);
    fragNormal = mat3(transpose(inverse(object.model))) * inNormal;
    fragUV = inUV;
}
