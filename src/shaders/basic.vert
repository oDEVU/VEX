#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// UBOs remain unchanged
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} camera;

layout(set = 0, binding = 1) uniform ModelUBO {
    mat4 model;
} object;

// Outputs
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out float fragDepth;

// Push constants with proper alignment
layout(push_constant) uniform PushConsts {
    float time;
    vec4 affineTransformX;
    vec4 affineTransformY;
    vec4 affineTransformZ;
    vec2 screenSize;
    float snapResolution;
    float jitterIntensity;
    int enablePS1Effects;
    vec4 color;
} push;

// PS1-style noise function
float ps1_noise(vec2 pos, float seed) {
    return fract(sin(dot(pos, vec2(12.9898, 78.233)) + seed) * 43758.5453);
}

void main() {
    // 1. PROPER vertex snapping (adjusted scale)
    vec3 processedPos = inPosition;
    if (bool(push.enablePS1Effects & 0x1)) {
        processedPos = floor(inPosition / 0.01) * 0.01; // 1cm grid
    }

    // 2. Transform to clip space
    vec4 worldPos = object.model * vec4(processedPos, 1.0);
    vec4 viewPos = camera.view * worldPos;
    vec4 clipPos = camera.proj * viewPos;

    // 3. PS1 JITTER
    if (bool(push.enablePS1Effects & 0x8)) {
        vec2 screenPos = (clipPos.xy / clipPos.w * 0.5 + 0.5) * push.screenSize;

        // Time parameters
        float jitterSpeed = 8.0; // Jitter cycles per second
        float phase = push.time * jitterSpeed;

        // Two alternating noise states
        float seed1 = floor(phase);
        float seed2 = seed1 + 1.0;

        vec2 jitter = vec2(
                ps1_noise(screenPos, seed1) - 0.5,
                ps1_noise(screenPos.yx, seed1 * 1.618) - 0.5
            ) * 2.0;

        screenPos += jitter * push.jitterIntensity;
        clipPos.xy = ((screenPos / push.screenSize) * 2.0 - 1.0) * clipPos.w;
    }

    gl_Position = clipPos;
    fragNormal = normalize(mat3(transpose(inverse(object.model))) * inNormal);
    fragUV = inUV;
    fragDepth = viewPos.z;
}
