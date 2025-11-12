struct VSInput {
    [[vk::location(0)]] float2 inPosition : POSITION;
    [[vk::location(1)]] float2 inUV : TEXCOORD0;
    [[vk::location(2)]] float4 inColor : COLOR;
    [[vk::location(3)]] float inTexIndex : TEXCOORD1;
};

struct VSOutput {
    float4 position : SV_POSITION;
    [[vk::location(0)]] float2 fragUV : TEXCOORD0;
    [[vk::location(1)]] float4 fragColor : COLOR;
    [[vk::location(2)]] nointerpolation float fragTexIndex : TEXCOORD1;
};

[[vk::push_constant]] cbuffer PushConstants {
    float4x4 ortho;
} pc;

VSOutput main(VSInput input) {
    VSOutput output;

    output.position = mul(pc.ortho, float4(input.inPosition, 0.0, 1.0));
    output.fragUV = input.inUV;
    output.fragColor = input.inColor;
    output.fragTexIndex = input.inTexIndex;

    return output;
}
