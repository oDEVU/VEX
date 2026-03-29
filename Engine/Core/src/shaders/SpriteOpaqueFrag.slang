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

[shader("fragment")]
float4 main(VSOutput input, float4 fragCoord: SV_Position) : SV_Target {
  float4 texColor =
      SampleMaterialTexture(input.fragUV, input.texID) * input.spriteColor;

  if (texColor.a < 0.05)
    discard;

  float3 lighting = float3(1.0, 1.0, 1.0);

  if (input.isUnlit == 0) {
    float diff = computeDiffuse(input.fragNormal, true, false);
    float3 ambient = scene.ambientLight.xyz * scene.ambientLightStrength;
    float3 sunLight = diff * scene.sunLight.xyz;
    lighting = saturate(ambient + sunLight + input.fragLights);
  }

  float4 finalColor = texColor * float4(lighting, 1.0);

  if (isEnabled(COLOR_QUANTIZATION))
    finalColor = applyQuantization(finalColor, fragCoord.xy, true);
  if (isEnabled(NTSC_ARTIFACTS))
    finalColor.rgb = applyNTSC(finalColor.rgb, input.fragUV, fragCoord.xy, true,
                               input.texID);

  return finalColor;
}
