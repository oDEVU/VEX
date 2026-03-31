import depends.uniforms;
import effects.ColorQuantization;

struct VSOutput {
  float4 position : SV_POSITION;
  [[vk::location(0)]]
  float2 uv : TEXCOORD0;
};

[[vk::binding(0, 1)]]
Sampler2D<float4> screenTexture;
[[vk::binding(1, 1)]]
Sampler2D<float4> accumTexture;
[[vk::binding(2, 1)]]
Sampler2D<float> revealTexture;
[[vk::binding(3, 1)]]
Sampler2D<float4> uiTexture;
[[vk::binding(4, 1)]]
Sampler3D<float4> colorLUT;

[shader("vertex")]
VSOutput vertMain(uint vertexID: SV_VertexID) {
  VSOutput output;
  output.uv = float2((vertexID << 1) & 2, vertexID & 2);
  output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
  return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
  float4 opaqueColor = screenTexture.Sample(input.uv);
  float4 accum = accumTexture.Sample(input.uv);
  float reveal = revealTexture.Sample(input.uv);

  float4 finalColor = opaqueColor;

  if (accum.a > 0.00001) {
    float3 averageColor = accum.rgb / max(accum.a, 0.00001);
    finalColor.rgb = opaqueColor.rgb * reveal + averageColor * (1.0 - reveal);
  }

  if (isEnabled(COLOR_QUANTIZATION)) {
    finalColor.rgb = colorLUT.Sample(finalColor.rgb).rgb;
  }

  if (isEnabled(SCREEN_DITHER)) {
    finalColor.rgb = applyDither(finalColor.rgb, input.position.xy);
  }

  float4 uiColor = uiTexture.Sample(input.uv);
  finalColor.rgb = lerp(finalColor.rgb, uiColor.rgb, uiColor.a);

  return finalColor;
}
