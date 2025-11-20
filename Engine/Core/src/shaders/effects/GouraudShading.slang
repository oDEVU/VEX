module GouraudShading;
import depends.uniforms;

public float computeDiffuse(float3 normal, bool useGouraud) {
  float3 lightDir = normalize(scene.sunDirection.xyz);
  return useGouraud ? max(dot(normalize(normal), lightDir), 0.0) : 0.0;
}
