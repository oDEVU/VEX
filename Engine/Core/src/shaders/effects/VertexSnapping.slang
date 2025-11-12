module VertexSnapping;

public float4 applySnapping(float4 worldPos, bool enable) {
    if (!enable) return worldPos;
    return float4(floor(worldPos.xyz / 0.01) * 0.01, worldPos.w);
}
