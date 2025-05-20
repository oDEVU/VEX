#version 450
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
// Optional: Add depth input if you want depth-based effects
// layout(location = 2) in float fragDepth;

layout(location = 0) out vec4 outColor;
// Optional: Add depth output if you need custom depth writing
// layout(location = 1) out float outDepth;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

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
        outColor = texture(texSampler, fragUV);

        // Optional: Debug visualization
        // outColor = vec4(fragUV, 0.0, 1.0); // UV visualization
        // outColor = vec4(vec3(gl_FragCoord.z), 1.0); // Depth visualization
    }

    // Optional: Write to depth buffer if needed
    // outDepth = gl_FragCoord.z;
}
