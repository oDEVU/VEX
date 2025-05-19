#version 450
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D texSampler;

// Add push constants for color
layout(push_constant) uniform PushConstants {
    vec4 color;
} push;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.2);

    // Use texture if UVs are valid, otherwise use push constant color
    if (fragUV.x < 0.0 || fragUV.y < 0.0) {
        outColor = push.color * diff;
    } else {
        // Debug: Output raw texture color
        outColor = texture(texSampler, fragUV);
        // Debug: Override with UV visualization
        // outColor = vec4(fragUV, 0.0, 1.0);
    }
}
