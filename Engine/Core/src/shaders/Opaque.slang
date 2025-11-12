import depends.uniforms;
import depends.math;

import effects.AffineWarping;
import effects.ColorQuantization;
import effects.NTSCArtifacts;
import effects.GouraudShading;
import effects.VertexSnapping;
import effects.VertexJitter;

[[vk::binding(0, 1)]] Texture2D<float4> texSampler;
[[vk::binding(0, 1)]] SamplerState texSamplerState;

struct VSInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_POSITION;
    [[vk::location(0)]] float3 fragNormal : NORMAL;
    [[vk::location(1)]] noperspective float2 fragUV : TEXCOORD0;
    [[vk::location(2)]] float fragDepth : TEXCOORD1;
    [[vk::location(3)]] float fragDiff : TEXCOORD2;
    [[vk::location(4)]] noperspective float2 fragUVNum : TEXCOORD3;
    [[vk::location(5)]] noperspective float fragInvW : TEXCOORD4;
};

struct FSInput {
    [[vk::location(0)]] float3 fragNormal : NORMAL;
    [[vk::location(1)]] noperspective float2 fragUV : TEXCOORD0;
    [[vk::location(2)]] float fragDepth : TEXCOORD1;
    [[vk::location(3)]] float fragDiff : TEXCOORD2;
    [[vk::location(4)]] noperspective float2 fragUVNum : TEXCOORD3;
    [[vk::location(5)]] noperspective float fragInvW : TEXCOORD4;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint vertId : SV_VertexID) {
    VSOutput output;

    float4 worldPos = mul(object.model, float4(input.position, 1.0));
    worldPos = applySnapping(worldPos, bool(push.enablePS1Effects & VERTEX_SNAPPING));

    float4 viewPos = mul(camera.view, worldPos);
    float4 clipPos = mul(camera.proj, viewPos);

    float2 screenPos = (clipPos.xy / clipPos.w * 0.5 + 0.5) * push.renderResolution;
    screenPos = applyJitter(screenPos, bool(push.enablePS1Effects & VERTEX_JITTER));

    clipPos.xy = (screenPos / push.windowResolution * 2.0 - 1.0) * clipPos.w;

    output.position = clipPos;

    float3x3 normalMat = transpose(inverse33((float3x3)object.model));
    output.fragNormal = normalize(mul(normalMat, input.normal));

    //output.fragNormal = normalize(mul((float3x3)transpose(object.model), input.normal));
    output.fragDiff = computeDiffuse(output.fragNormal, bool(push.enablePS1Effects & GOURAUD_SHADING));
    output.fragUV = input.uv;
    output.fragDepth = viewPos.z;
    float invW = 1.0 / clipPos.w;
    output.fragUVNum = input.uv * invW;
    output.fragInvW = invW;

    return output;
}

[shader("fragment")]
float4 fragMain(FSInput input, float4 fragCoord : SV_Position) : SV_Target {
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
