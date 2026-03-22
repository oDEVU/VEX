import depends.uniforms;
import effects.Crt;

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

[shader("vertex")]
VSOutput vertMain(uint vertexID: SV_VertexID) {
  VSOutput output;
  output.uv = float2((vertexID << 1) & 2, vertexID & 2);
  output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
  return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
  float4 opaqueColor;

  if (isEnabled(NTSC_ARTIFACTS)) {
    float3 crtColor = ApplyCRT(input.uv, input.position.xy, screenTexture);
    opaqueColor = float4(crtColor, 1.0);
  } else {
    opaqueColor = screenTexture.Sample(input.uv);
  }

  float4 accum = accumTexture.Sample(input.uv);
  float reveal = revealTexture.Sample(input.uv);

  float4 finalColor = opaqueColor;

  if (accum.a > 0.00001) {
    float3 averageColor = accum.rgb / max(accum.a, 0.00001);
    finalColor.rgb = opaqueColor.rgb * reveal + averageColor * (1.0 - reveal);
  }

  return finalColor;
}
