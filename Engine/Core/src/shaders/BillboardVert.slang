import depends.uniforms;
import depends.math;
import effects.VertexSnapping;
import effects.VertexJitter;

struct BillboardPush {
  float3 position;
  float scaleX;
  float scaleY;
  uint textureID;
  uint isUnlit;
  float padding;
  float4 color;
};

[[vk::push_constant]]
BillboardPush pushSprite;

struct VSOutput {
  float4 position : SV_POSITION;
  [[vk::location(0)]]
  float3 fragNormal : NORMAL;
  [[vk::location(1)]]
  noperspective float2 fragUV : TEXCOORD0;
  [[vk::location(2)]]
  float fragDepth : TEXCOORD1;
  [[vk::location(3)]]
  float fragDiff : TEXCOORD2;
  [[vk::location(4)]]
  float3 fragLights : TEXCOORD3;
  [[vk::location(5)]]
  noperspective float2 fragUVNum : TEXCOORD4;
  [[vk::location(6)]]
  noperspective float fragInvW : TEXCOORD5;
  [[vk::location(7)]]
  float4 spriteColor : COLOR0;
  [[vk::location(8)]]
  nointerpolation uint isUnlit : BLENDINDICES0;
  [[vk::location(9)]]
  nointerpolation uint texID : BLENDINDICES1;
};

[shader("vertex")]
VSOutput main(uint vertexID: SV_VertexID) {
  VSOutput output;

  float2 positions[6] = { float2(-0.5, -0.5), float2(0.5, -0.5),
                          float2(0.5, 0.5),   float2(0.5, 0.5),
                          float2(-0.5, 0.5),  float2(-0.5, -0.5) };

  float2 uvs[6] = { float2(0.0, 1.0), float2(1.0, 1.0), float2(1.0, 0.0),
                    float2(1.0, 0.0), float2(0.0, 0.0), float2(0.0, 1.0) };

  float2 localPos = positions[vertexID];
  output.fragUV = uvs[vertexID];

  float3 cameraRight = scene.view[0].xyz;
  float3 cameraUp = scene.view[1].xyz;

  float3 vertexPos = pushSprite.position +
                     (cameraRight * localPos.x * pushSprite.scaleX) +
                     (cameraUp * localPos.y * pushSprite.scaleY);

  float4 worldPos =
      applySnapping(float4(vertexPos, 1.0), isEnabled(VERTEX_SNAPPING));
  float4 viewPos = mul(scene.view, worldPos);
  float4 clipPos = mul(scene.proj, viewPos);

  float2 screenPos =
      (clipPos.xy / clipPos.w * 0.5 + 0.5) * scene.renderResolution;
  screenPos = applyJitter(screenPos, isEnabled(VERTEX_JITTER));
  clipPos.xy = (screenPos / scene.renderResolution * 2.0 - 1.0) * clipPos.w;

  output.position = clipPos;
  output.fragNormal = -scene.view[2].xyz;
  output.fragDepth = viewPos.z;
  output.fragLights = float3(0.0, 0.0, 0.0);
  output.fragDiff = 1.0;

  float invW = 1.0 / clipPos.w;
  output.fragUVNum = output.fragUV * invW;
  output.fragInvW = invW;

  output.spriteColor = pushSprite.color;
  output.isUnlit = pushSprite.isUnlit;
  output.texID = pushSprite.textureID;

  return output;
}
