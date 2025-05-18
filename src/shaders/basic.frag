#version 450
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D texSampler;
layout(push_constant) uniform PushConstants {
    vec3 color;
} push;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.2);

    outColor = texture(texSampler, fragUV) * vec4(push.color * diff, 1.0);
}
