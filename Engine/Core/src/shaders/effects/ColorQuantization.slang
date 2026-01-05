module ColorQuantization;
import depends.uniforms;

static const float4x4 psxDitherTable = float4x4(
    float4(0, 8, 2, 10), float4(12, 4, 14, 6),
    float4(3, 11, 1, 9), float4(15, 7, 13, 5)
);

public float3 applyDither(float3 color, float2 pixelCoord) {
    color *= 255.0;
    int col = int(fmod(pixelCoord.x, 4.0));
    int row = int(fmod(pixelCoord.y, 4.0));
    float ditherVal = psxDitherTable[col][row];
    color += (ditherVal / 2.0 - 4.0);
    color = max(color, 0.0);
    int3 c = int3(color) & int3(0xf8);
    color = lerp(float3(c), float3(0xf8), step(float3(0xf8), color));
    return color / 255.0;
}

public float4 applyQuantization(float4 color, float2 pixelCoord, bool enable) {
    if (!enable) return color;
    float textureBpc = pow(2.0, 5.0); // 5-bit depth
    color.rgb = round(color.rgb * textureBpc) / textureBpc;
    //color.rgb = dither(color.rgb, pixelCoord);
    return color;
}
