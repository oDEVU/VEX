#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in float fragTexIndex;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    if (fragTexIndex < 0.0) {
        outColor = fragColor;
    } else {
        outColor = texture(texSampler, fragUV) * fragColor;
    }
}
