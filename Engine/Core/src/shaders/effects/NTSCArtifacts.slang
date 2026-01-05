module NTSCArtifacts;
import depends.uniforms;
import depends.texsampler;

public float3 applyNTSC(float3 texColor, float2 uv, float2 pixelCoord,
                        bool enable, int textureID) {
  if (!enable)
    return texColor;
  float2 bleedUV = float2(uv.x + sin(scene.time * 0.5) * 0.0005, uv.y);
  float3 bleedColor = SampleMaterialTexture(bleedUV, textureID).rgb;
  texColor = lerp(texColor, bleedColor, 0.15);
  float scanline = lerp(0.95, 1.0, fmod(pixelCoord.y, 2.0));
  texColor *= scanline;
  float noise =
      frac(sin(dot(pixelCoord, float2(12.9898, 78.233))) * 43758.5453);
  texColor += (noise - 0.5) * 0.01;
  return texColor;
}
