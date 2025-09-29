#version 450
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in float fragDepth;
layout(location = 3) in float fragDiff;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

// Push constants match vertex shader
layout(push_constant) uniform PushConsts {
    float snapResolution;
    float jitterIntensity;
    int enablePS1Effects;
    vec4 color;

    // Resolution control
    float time;
    vec2 renderResolution;
    vec2 windowResolution;
    float upscaleRatio;
    int renderingMode;
} push;

mat4 psx_dither_table = mat4(
        0, 8, 2, 10,
        12, 4, 14, 6,
        3, 11, 1, 9,
        15, 7, 13, 5
    );

vec3 dither(vec3 color, vec2 p) {
    //extrapolate 16bit color float to 16bit integer space
    color *= 255.;

    //get dither value from dither table (by indexing it with column and row offsets)
    int col = int(mod(p.x, 4.));
    int row = int(mod(p.y, 4.));
    float dither = psx_dither_table[col][row];

    //dithering process as described in PSYDEV SDK documentation
    color += (dither / 2. - 4.);

    //clamp to 0
    color = max(color, 0.);

    //truncate to 5bpc precision via bitwise AND operator, and limit value max to prevent wrapping.
    //PS1 colors in default color mode have a maximum integer value of 248 (0xf8)
    ivec3 c = ivec3(color) & ivec3(0xf8);
    color = mix(vec3(c), vec3(0xf8), step(vec3(0xf8), color));

    //bring color back to floating point number space
    color /= 255.;
    return color;
}

void main() {
    vec2 uv = vec2(0, 0);
    if (bool(push.enablePS1Effects & 0x2)) {
        uv = fragUV;
    } else {
        uv = fragUV;
    }

    vec2 pixelCoord = gl_FragCoord.xy;
    vec4 pushColor = push.color;
    vec4 texColor = texture(texSampler, uv);

    if (bool(push.enablePS1Effects & 0x4)) {
        int texture_depth = 5;
        float texture_bpc = pow(2.0, float(texture_depth));

        if (fragUV.x < 0.0 || fragUV.y < 0.0) {
            pushColor.rgb = round(pushColor.rgb * texture_bpc) / texture_bpc;
            pushColor.rgb = dither(pushColor.rgb, pixelCoord);
        } else {
            texColor.rgb = round(texColor.rgb * texture_bpc) / texture_bpc;
            texColor.rgb = dither(texColor.rgb, pixelCoord);
        }
    }

    if (bool(push.enablePS1Effects & 0x10)) {
        vec2 bleedUV = vec2(fragUV.x + sin(push.time * 0.5) * 0.0005, fragUV.y);
        vec3 bleedColor = texture(texSampler, bleedUV).rgb;
        texColor.rgb = mix(texColor.rgb, bleedColor, 0.15);

        float scanline = mix(0.95, 1.0, mod(gl_FragCoord.y, 2.0));
        texColor.rgb *= scanline;

        float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
        texColor.rgb += (noise - 0.5) * 0.01;
    }

    float diff = 0;

    if (bool(push.enablePS1Effects & 0x20)) {
        diff = fragDiff;
    } else {
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        diff = max(dot(normalize(fragNormal), lightDir), 0.1);
    }

    // check if models are untextured
    if (fragUV.x < 0.0 || fragUV.y < 0.0) {
        // untextured models use push.color
        outColor = pushColor * diff;
    } else {
        outColor = texColor * diff;
    }
}
