module AffineWarping;

public float2 applyAffineUV(float2 fragUV, float2 fragUVNum, float fragInvW, bool enable) {
    return enable ? fragUV : (fragUVNum / fragInvW);
}
