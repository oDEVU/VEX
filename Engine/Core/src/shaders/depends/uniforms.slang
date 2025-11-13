module uniforms;

public [[vk::binding(0, 0)]] cbuffer CameraUBO {
    public float4x4 view;
    public float4x4 proj;
} camera;

public [[vk::binding(1, 0)]] cbuffer ModelUBO {
    public float4x4 model;
} object;

public struct Light {
    public float3 position; // xyz
    public float radius;
    public float3 color;    // rgb
    public float intensity; // a
};

public [[vk::binding(2, 0)]] cbuffer ObjectLightsUBO {
    public uint lightCount;
    public Light lights[255];
} objectLights;

public [[vk::push_constant]] cbuffer PushConstants {
    public float snapResolution;
    public float jitterIntensity;
    public int enablePS1Effects;
    public float4 color;
    public float time;
    public float2 renderResolution;
    public float2 windowResolution;
    public float upscaleRatio;
    public int renderingMode;
    public float4 ambientLight;
    public float ambientLightStrength;
    public float4 sunLight;
    public float4 sunDirection;
} push;

public bool isEnabled(uint flag) {
    return bool(push.enablePS1Effects & flag);
}

public static const int VERTEX_SNAPPING = 0x1;
public static const int AFFINE_WARPING = 0x2;
public static const int COLOR_QUANTIZATION = 0x4;
public static const int VERTEX_JITTER = 0x8;
public static const int NTSC_ARTIFACTS = 0x10;
public static const int GOURAUD_SHADING = 0x20;
