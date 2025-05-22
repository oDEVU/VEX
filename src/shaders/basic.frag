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
    const float[16] bayerMatrix = float[](
            0.0 / 16.0, 8.0 / 16.0, 2.0 / 16.0, 10.0 / 16.0,
            12.0 / 16.0, 4.0 / 16.0, 14.0 / 16.0, 6.0 / 16.0,
            3.0 / 16.0, 11.0 / 16.0, 1.0 / 16.0, 9.0 / 16.0,
            15.0 / 16.0, 7.0 / 16.0, 13.0 / 16.0, 5.0 / 16.0
        );
    ivec2 p = ivec2(mod(pos, 4.0));
    return bayerMatrix[p.y * 4 + p.x];
}

vec3 rgbToHsl(vec3 c) {
    float minVal = min(min(c.r, c.g), c.b);
    float maxVal = max(max(c.r, c.g), c.b);
    float delta = maxVal - minVal;

    float h = 0.0;
    float s = 0.0;
    float l = (maxVal + minVal) * 0.5;

    if (delta != 0.0) {
        s = l < 0.5 ? delta / (maxVal + minVal) : delta / (2.0 - maxVal - minVal);
        if (c.r == maxVal) {
            h = (c.g - c.b) / delta + (c.g < c.b ? 6.0 : 0.0);
        } else if (c.g == maxVal) {
            h = (c.b - c.r) / delta + 2.0;
        } else {
            h = (c.r - c.g) / delta + 4.0;
        }
        h /= 6.0;
    }
    return vec3(h, s, l);
}

vec3 hslToRgb(vec3 hsl) {
    float h = hsl.x;
    float s = hsl.y;
    float l = hsl.z;

    float c = (1.0 - abs(2.0 * l - 1.0)) * s;
    float x = c * (1.0 - abs(mod(h * 6.0, 2.0) - 1.0));
    float m = l - c * 0.5;

    vec3 rgb;
    if (h < 1.0 / 6.0) {
        rgb = vec3(c, x, 0.0);
    } else if (h < 2.0 / 6.0) {
        rgb = vec3(x, c, 0.0);
    } else if (h < 3.0 / 6.0) {
        rgb = vec3(0.0, c, x);
    } else if (h < 4.0 / 6.0) {
        rgb = vec3(0.0, x, c);
    } else if (h < 5.0 / 6.0) {
        rgb = vec3(x, 0.0, c);
    } else {
        rgb = vec3(c, 0.0, x);
    }

    rgb += m;
    return clamp(rgb, 0.0, 1.0);
}

vec3 advancedDither(vec3 color, vec2 fragCoord) {
    vec3 hsl = rgbToHsl(color);

    // Hue quantization (8 steps)
    const float hueSteps = 8.0;
    float hueStep = 1.0 / hueSteps;
    float q1_hue = floor(hsl.x * hueSteps) * hueStep;
    float q2_hue = q1_hue + hueStep;
    if (q2_hue >= 1.0) q2_hue -= 1.0;

    float hue_diff = hsl.x - q1_hue;
    if (hue_diff < 0.0) hue_diff += 1.0;
    float hueDiff = hue_diff / hueStep;

    // Lightness quantization (4 steps)
    const float lightnessSteps = 4.0;
    float lightnessStepSize = 1.0 / lightnessSteps;

    float l_lower = max(hsl.z - 0.125, 0.0);
    float l_upper = min(hsl.z + 0.124, 1.0);

    float l1 = floor(l_lower * lightnessSteps + 0.5) * lightnessStepSize;
    float l2 = floor(l_upper * lightnessSteps + 0.5) * lightnessStepSize;

    if (l1 == l2) {
        if (l2 < 1.0 - lightnessStepSize) {
            l2 += lightnessStepSize;
        } else {
            l1 -= lightnessStepSize;
        }
    }

    float lightnessDiff = (hsl.z - l1) / (l2 - l1);

    float d = bayerDither(fragCoord);
    float selectedHue = (hueDiff < d) ? q1_hue : q2_hue;
    float selectedLightness = (lightnessDiff < d) ? l1 : l2;

    return hslToRgb(vec3(selectedHue, hsl.y, selectedLightness));
}

mat3 getAffineTransform() {
    return mat3(
        push.affineTransformX.xyz,
        push.affineTransformY.xyz,
        push.affineTransformZ.xyz
    );
}

void main() {
    vec2 uv = fragUV;
    if (bool(push.enablePS1Effects & 0x2)) {
        ivec2 texSize = textureSize(texSampler, 0);
        vec2 pixelUV = uv * texSize;
        pixelUV = floor(pixelUV) + fract(pixelUV * 1.005);
        uv = pixelUV / texSize;
    }

    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.2);

    vec2 pixelCoord = gl_FragCoord.xy;
    vec4 pushColor = push.color;
    vec4 texColor = texture(texSampler, uv);

    if (bool(push.enablePS1Effects & 0x4)) {
        if (fragUV.x < 0.0 || fragUV.y < 0.0) {
            pushColor.rgb = advancedDither(pushColor.rgb, pixelCoord);
        } else {
            texColor.rgb = advancedDither(texColor.rgb, pixelCoord);
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

    // check if models are untextured
    if (fragUV.x < 0.0 || fragUV.y < 0.0) {
        // untextured models use push.color
        outColor = pushColor * diff;
    } else {
        outColor = texColor * diff;
    }
}
