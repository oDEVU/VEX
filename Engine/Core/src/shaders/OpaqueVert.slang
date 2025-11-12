import depends.uniforms;
import depends.math;
import effects.VertexSnapping;
import effects.VertexJitter;
import effects.GouraudShading;

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

VSOutput main(VSInput input, uint vertId : SV_VertexID) {
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
