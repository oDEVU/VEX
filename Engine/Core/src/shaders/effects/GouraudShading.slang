module GouraudShading;
import depends.uniforms;

public float computeDiffuse(float3 normal, bool useGouraud, bool isTwoSided) {
  float3 lightDir = normalize(scene.sunDirection.xyz);

  float NdotL = dot(normalize(normal), lightDir);
  float diffuseTerm = isTwoSided ? abs(NdotL) : max(NdotL, 0.0);

  return useGouraud ? diffuseTerm : 0.0;
}
