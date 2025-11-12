module VertexJitter;
import depends.uniforms;

float ps1Noise(float2 pos, float seed) {
    return frac(sin(dot(pos, float2(12.9898, 78.233)) + seed) * 43758.5453);
}

public float2 applyJitter(float2 screenPos, bool enable) {
    if (!enable) return screenPos;
    float jitterSpeed = 8.0;
    float phase = push.time * jitterSpeed;
    float seed1 = floor(phase);
    float2 jitter = float2(
        ps1Noise(screenPos, seed1) - 0.5,
        ps1Noise(screenPos.yx, seed1 * 1.618) - 0.5
    ) * 2.0;
    return screenPos + jitter * push.jitterIntensity;
}
