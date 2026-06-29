import depends.uniforms;
import depends.texsampler;
import effects.ColorQuantization;
import effects.NTSCArtifacts;
import effects.GouraudShading;

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

struct WBOITOutput {
  [[vk::location(0)]]
  float4 accumulation : SV_Target0;
  [[vk::location(1)]]
  float revealage : SV_Target1;
};

[shader("fragment")]
WBOITOutput main(VSOutput input, float4 fragCoord: SV_Position) {
  float4 texColor =
      SampleMaterialTexture(input.fragUV, input.texID) * input.spriteColor;

  if (texColor.a < 0.01)
    discard;

  float3 lighting = float3(1.0, 1.0, 1.0);
  if (input.isUnlit == 0) {
    float diff = computeDiffuse(input.fragNormal, true, true);
    float3 ambient = scene.ambientLight.xyz * scene.ambientLightStrength;
    float3 sunLight = diff * scene.sunLight.xyz;
    lighting = saturate(ambient + sunLight + input.fragLights);
  }

  float4 finalColor = texColor * float4(lighting, 1.0);

  /*
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
  */
  float a = finalColor.a;
  float z = fragCoord.z;
  float weight = clamp(
      pow(min(1.0, a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - z, 3.0), 1e-2, 3e3);

  WBOITOutput output;
  output.accumulation = float4(finalColor.rgb * a, a) * weight;
  output.revealage = a;
  return output;
}
