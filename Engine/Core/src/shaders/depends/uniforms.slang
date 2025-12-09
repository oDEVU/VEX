module uniforms;

public[[vk::binding(0, 0)]]
cbuffer SceneUBO {
  public float4x4 view;
  public float4x4 proj;

  public float4 ambientLight;
  public float4 sunLight;
  public float4 sunDirection;

  public float2 renderResolution;
  public float2 windowResolution;

  public float snapResolution;
  public float jitterIntensity;
  public int enablePS1Effects;
  public float time;
  public int frame;
  public float upscaleRatio;
  public float ambientLightStrength;
}
scene;

public struct Light {
  public float3 position; // xyz
  public float radius;
  public float3 color;    // rgb
  public float intensity; // a
};

public[[vk::binding(1, 0)]]
cbuffer ObjectLightsUBO {
  public uint lightCount;
  public Light lights[255];
}
objectLights;

public[[vk::push_constant]]
cbuffer PushConstants {
  public float4 color;
  public float4x4 model;
}
push;

public bool isEnabled(uint flag) { return bool(scene.enablePS1Effects & flag); }

public static const int VERTEX_SNAPPING = 0x1;
public static const int AFFINE_WARPING = 0x2;
public static const int COLOR_QUANTIZATION = 0x4;
public static const int VERTEX_JITTER = 0x8;
public static const int NTSC_ARTIFACTS = 0x10;
public static const int GOURAUD_SHADING = 0x20;
public static const int TEXTURE_QUANTIZATION = 0x40;
public static const int SCREEN_DITHER = 0x80;

public bool HasInValidUV(float2 uv) { return uv.x <= -10000.0; }
