import depends.uniforms;
import depends.math;
import depends.texsampler;

import effects.AffineWarping;
import effects.ColorQuantization;
import effects.NTSCArtifacts;
import effects.GouraudShading;
import effects.VertexSnapping;
import effects.VertexJitter;

struct VSInput {
  [[vk::location(0)]]
  float3 position : POSITION;
  [[vk::location(1)]]
  float3 normal : NORMAL;
  [[vk::location(2)]]
  float2 uv : TEXCOORD0;
};

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
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint vertId: SV_VertexID) {
  VSOutput output;

  float4 worldPos = mul(push.model, float4(input.position, 1.0));
  worldPos = applySnapping(worldPos, isEnabled(VERTEX_SNAPPING));

  float4 viewPos = mul(scene.view, worldPos);
  float4 clipPos = mul(scene.proj, viewPos);

  float2 screenPos =
      (clipPos.xy / clipPos.w * 0.5 + 0.5) * scene.renderResolution;
  screenPos = applyJitter(screenPos, isEnabled(VERTEX_JITTER));

  clipPos.xy = (screenPos / scene.renderResolution * 2.0 - 1.0) * clipPos.w;

  output.position = clipPos;

  float3x3 normalMat = transpose(inverse33((float3x3)push.model));
  output.fragNormal = normalize(mul(normalMat, input.normal));

  float3 dynamicLight = float3(0.0, 0.0, 0.0);

  for (uint i = 0; i < objectLights.lightCount; ++i) {
    Light light = objectLights.lights[i];
    float3 toLight = light.position - worldPos.xyz;
    float dist = length(toLight);

    if (dist < light.radius) {
      float3 L = normalize(toLight);
      float NdotL = abs(dot(output.fragNormal, L));
      float distSqr = dist * dist;
      float atten = light.intensity / max(distSqr, 0.01);
      dynamicLight += NdotL * atten * light.color * light.intensity;
    }
  }

  output.fragLights = dynamicLight;
  output.fragDiff =
      computeDiffuse(output.fragNormal, isEnabled(GOURAUD_SHADING), false);
  output.fragUV = input.uv;
  output.fragDepth = viewPos.z;
  float invW = 1.0 / clipPos.w;
  output.fragUVNum = input.uv * invW;
  output.fragInvW = invW;

  return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input, float4 fragCoord: SV_Position) : SV_Target {
  float2 uv = applyAffineUV(input.fragUV, input.fragUVNum, input.fragInvW,
                            isEnabled(AFFINE_WARPING));
  float2 pixelCoord = fragCoord.xy;

  float4 texColor = SampleMaterialTexture(uv, push.textureID);
  float4 pushColor = push.color;
  bool isUntextured = HasInValidUV(uv);

  if (!isUntextured) {
    texColor = texColor * pushColor;
  }

  if (isUntextured) {
    if (pushColor.a < 0.5)
      discard;
  } else {
    if (texColor.a < 0.5)
      discard;
  }

  if (isEnabled(TEXTURE_QUANTIZATION)) {
    float4 c = isUntextured ? pushColor : texColor;
    c = applyQuantization(c, pixelCoord, true);
    if (isUntextured)
      pushColor = c;
    else
      texColor = c;
  }

  texColor.rgb = applyNTSC(texColor.rgb, uv, pixelCoord,
                           isEnabled(NTSC_ARTIFACTS), push.textureID);

  float diff = isEnabled(GOURAUD_SHADING)
                   ? input.fragDiff
                   : computeDiffuse(input.fragNormal, true, false);

  // diff += input.fragLights;

  float3 ambient = scene.ambientLight.xyz * scene.ambientLightStrength;
  float3 sunLight = diff * scene.sunLight.xyz;
  // float3 lighting = ambient + (diff * push.sunLight.xyz);
  float3 lighting = ambient + sunLight + input.fragLights;
  lighting = saturate(lighting);

  float4 finalColor = isUntextured ? (pushColor * float4(lighting, 1.0))
                                   : (texColor * float4(lighting, 1.0));

  if (isEnabled(COLOR_QUANTIZATION)) {
    float4 c = finalColor;
    c = applyQuantization(c, pixelCoord, true);
    finalColor = c;
  }

  if (isEnabled(SCREEN_DITHER)) {
    float3 c = finalColor.rgb;
    c = applyDither(c, pixelCoord);
    finalColor.rgb = c;
  }

  return finalColor;
}
