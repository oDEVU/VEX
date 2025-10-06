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
layout(location = 1) noperspective out vec2 fragUV;
layout(location = 2) out float fragDepth;
layout(location = 3) out float fragDiff;
layout(location = 4) noperspective out vec2 fragUVNum;
layout(location = 5) noperspective out float fragInvW;

layout(push_constant) uniform PushConsts {
    float snapResolution;
    float jitterIntensity;
    int enablePS1Effects;
    vec4 color;

    // Resolution control
    float time;
    vec2 renderResolution;
    vec2 windowResolution;
    float upscaleRatio;
    int renderingMode;

    // Lighting
    vec4 ambientLight;
    float ambientLightStrength;
    vec4 sunLight;
    vec4 sunDirection;
} push;

// PS1-style noise function
float ps1_noise(vec2 pos, float seed) {
    return fract(sin(dot(pos, vec2(12.9898, 78.233)) + seed) * 43758.5453);
}

void main() {
    vec4 worldPos = object.model * vec4(inPosition, 1.0);

    // 1. PROPER vertex snapping
    if (bool(push.enablePS1Effects & 0x1)) {
        worldPos.xyz = floor(worldPos.xyz / 0.01) * 0.01; // 1cm grid
    }

    // 2. Transform to clip space
    vec4 viewPos = camera.view * worldPos;
    vec4 clipPos = camera.proj * viewPos;
    vec4 tstPos = camera.proj * worldPos;

    vec2 screenPos = (clipPos.xy / clipPos.w * 0.5 + 0.5) * push.renderResolution;

    // 3. PS1 JITTER
    if (bool(push.enablePS1Effects & 0x8)) {
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
    }

    clipPos.xy = ((screenPos / push.windowResolution) * 2.0 - 1.0) * clipPos.w;

    gl_Position = clipPos;
    fragNormal = normalize(mat3(transpose(inverse(object.model))) * inNormal);

    //gourard shading
    vec3 lightDir = normalize(push.sunDirection.xyz);
    float diff = max(dot(normalize(fragNormal), lightDir), 0.0);

    fragDiff = diff;

    float inv_w = 1.0 / clipPos.w;
    fragUVNum = inUV * inv_w;
    fragInvW = inv_w;
    fragUV = inUV;

    fragDepth = viewPos.z;
}
