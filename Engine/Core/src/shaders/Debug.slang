import depends.uniforms;

struct VSInput {
  [[vk::location(0)]]
  float3 position : POSITION;
  [[vk::location(1)]]
  float4 color : COLOR;
};

struct VSOutput {
  float4 position : SV_POSITION;
  [[vk::location(0)]]
  float4 color : COLOR;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
  VSOutput output;
  float4 viewPos = mul(scene.view, float4(input.position, 1.0));
  output.position = mul(scene.proj, viewPos);
  output.color = input.color;
  return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target { return input.color; }
