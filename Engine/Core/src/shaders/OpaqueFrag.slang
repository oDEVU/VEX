import depends.uniforms;
import effects.AffineWarping;
import effects.ColorQuantization;
import effects.NTSCArtifacts;
import effects.GouraudShading;

[[vk::binding(0, 1)]] Texture2D<float4> texSampler;
[[vk::binding(0, 1)]] SamplerState texSamplerState;

struct FSInput {
    [[vk::location(0)]] float3 fragNormal : NORMAL;
    [[vk::location(1)]] noperspective float2 fragUV : TEXCOORD0;
    [[vk::location(2)]] float fragDepth : TEXCOORD1;
    [[vk::location(3)]] float fragDiff : TEXCOORD2;
    [[vk::location(4)]] noperspective float2 fragUVNum : TEXCOORD3;
    [[vk::location(5)]] noperspective float fragInvW : TEXCOORD4;
};

float4 main(FSInput input, float4 fragCoord : SV_Position) : SV_Target {
    float2 uv = applyAffineUV(input.fragUV, input.fragUVNum, input.fragInvW, bool(push.enablePS1Effects & AFFINE_WARPING));
    float2 pixelCoord = fragCoord.xy;

    float4 texColor = texSampler.Sample(texSamplerState, uv);
    float4 pushColor = push.color;
    bool isUntextured = (uv.x < 0.0 || uv.y < 0.0);

    if (bool(push.enablePS1Effects & COLOR_QUANTIZATION)) {
        if (isUntextured) {
            pushColor = applyQuantization(pushColor, pixelCoord, true);
        } else {
            texColor = applyQuantization(texColor, pixelCoord, true);
        }
    }

    texColor.rgb = applyNTSC(texColor.rgb, uv, pixelCoord, bool(push.enablePS1Effects & NTSC_ARTIFACTS));

    float diff;
    if (bool(push.enablePS1Effects & GOURAUD_SHADING)) {
        diff = input.fragDiff;
    } else {
        diff = computeDiffuse(input.fragNormal, true);
    }

    float3 ambient = push.ambientLight.xyz * push.ambientLightStrength;
    float3 lighting = ambient + (diff * push.sunLight.xyz);

    return isUntextured ? (pushColor * float4(lighting, 1.0)) : (texColor * float4(lighting, 1.0));
}
