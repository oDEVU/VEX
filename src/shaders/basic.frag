#version 450
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

// Push constants match vertex shader
layout(push_constant) uniform PushConsts {
    float time;
    vec4 affineTransformX;
    vec4 affineTransformY;
    vec4 affineTransformZ;
    vec2 screenSize;
    float snapResolution;
    float jitterIntensity;
    int enablePS1Effects;
    vec4 color;
} push;

// PS1's 4x4 Bayer dither matrix
float bayerDither(vec2 pos) {
    // PS1's 4x4 Bayer matrix with pre-divided values
    const float[16] bayerMatrix = float[](
            0.0 / 16.0, 8.0 / 16.0, 2.0 / 16.0, 10.0 / 16.0,
            12.0 / 16.0, 4.0 / 16.0, 14.0 / 16.0, 6.0 / 16.0,
            3.0 / 16.0, 11.0 / 16.0, 1.0 / 16.0, 9.0 / 16.0,
            15.0 / 16.0, 7.0 / 16.0, 13.0 / 16.0, 5.0 / 16.0
        );

    ivec2 p = ivec2(mod(pos, 4.0));
    return bayerMatrix[p.y * 4 + p.x];
}

vec4 ps1Quantize(vec4 color, vec2 fragCoord) {
    const float levels = 16.0;

    // Apply Bayer dither before quantization
    float dither = bayerDither(fragCoord) - 0.5;
    color.rgb += dither / levels;

    // Hard quantization
    return floor(color * levels) / levels;
}

mat3 getAffineTransform() {
    return mat3(
        push.affineTransformX.xyz,
        push.affineTransformY.xyz,
        push.affineTransformZ.xyz
    );
}

void main() {
    // 1. Affine Texture Warping
    vec2 uv = fragUV;
    if (bool(push.enablePS1Effects & 0x2)) {
        // Convert to texture pixel coordinates
        ivec2 texSize = textureSize(texSampler, 0);
        vec2 pixelUV = uv * texSize;

        // PS1-style sub-pixel distortion
        pixelUV = floor(pixelUV) + fract(pixelUV * 1.005);
        uv = pixelUV / texSize;
    }

    // 2. Lighting (Simplified)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.2);

    // 3. Texture Sampling with Quantization
    vec2 pixelCoord = gl_FragCoord.xy;
    vec4 texColor = texture(texSampler, uv);
    if (bool(push.enablePS1Effects & 0x4)) {
        texColor = ps1Quantize(texColor, pixelCoord);
    }

    // ntsc artifacts
    if (bool(push.enablePS1Effects & 0x10)) { // Use bit 0x10 for NTSC effects
        // 1. Gentle color bleed (horizontal only)
        vec2 bleedUV = vec2(fragUV.x + sin(push.time * 0.5) * 0.0005, fragUV.y);
        vec3 bleedColor = texture(texSampler, bleedUV).rgb;
        texColor.rgb = mix(texColor.rgb, bleedColor, 0.15); // Reduced from 0.3

        // 2. Subtle scanlines (every 2nd line)
        float scanline = mix(0.95, 1.0, mod(gl_FragCoord.y, 2.0));
        texColor.rgb *= scanline;

        // 3. Mild composite noise
        float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
        texColor.rgb += (noise - 0.5) * 0.01; // Very subtle noise
    }

    //outColor = texColor * diff;

    if (fragUV.x < 0.0 || fragUV.y < 0.0) {
        outColor = vec4(vec3(push.color * bayerDither(gl_FragCoord.xy)), 1.0);
    } else {
        outColor = texColor * diff;
    }
}
