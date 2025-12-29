module texsampler;

#ifdef BINDLESS
public[[vk::binding(0, 1)]]
Sampler2D<float4> globalTextures[];

public float4 SampleMaterialTexture(float2 uv, int id) {
  return globalTextures[NonUniformResourceIndex(id)].Sample(uv);
}
#else
public[[vk::binding(0, 1)]]
Sampler2D<float4> texSampler;

public float4 SampleMaterialTexture(float2 uv, int id) {
  return texSampler.Sample(uv);
}
#endif
