struct FSInput {
    [[vk::location(0)]] float2 fragUV : TEXCOORD0;
    [[vk::location(1)]] float4 fragColor : COLOR;
    [[vk::location(2)]] nointerpolation float fragTexIndex : TEXCOORD1;
};

struct FSOutput {
    float4 outColor : SV_Target;
};

[[vk::binding(0, 1)]] Texture2D<float4> texSampler;
[[vk::binding(0, 1)]] SamplerState texSamplerState;

FSOutput main(FSInput input) {
    FSOutput output;

    if (input.fragTexIndex < 0.0) {
        output.outColor = input.fragColor;
    } else {
        output.outColor = texSampler.Sample(texSamplerState, input.fragUV) * input.fragColor;
    }

    return output;
}
