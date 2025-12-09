module Crt;
import depends.uniforms;
import depends.texsampler;

// --- CONFIGURATION ---
static const float SCANLINE_OPACITY = 0.4;
static const float BLUR_WIDTH = 1.0;
static const float CONVERGENCE_OFFSET = 0.5;

// --- BLOOM SETTINGS ---
static const float HALATION_INTENSITY = 0.5;
static const float HALATION_RADIUS = 2.5;
static const float HALATION_THRESHOLD = 0.1;

static const float NTSC_ARTIFACT_STR = 0.6;
static const float MASK_INTENSITY = 0.3;
static const float BRIGHTNESS_BOOST = 1.4;
static const float SATURATION = 1.1;

static const float3x3 RGB_to_YIQ =
    float3x3(0.299, 0.587, 0.114, 0.596, -0.274, -0.322, 0.211, -0.523, 0.311);
static const float3x3 YIQ_to_RGB =
    float3x3(1.0, 0.956, 0.621, 1.0, -0.272, -0.647, 1.0, -1.106, 1.703);

static const float weights[5] = { 0.061, 0.242, 0.383, 0.242, 0.061 };
static const float offsets[5] = { -2.0, -1.0, 0.0, 1.0, 2.0 };

float luma(float3 c) { return dot(c, float3(0.299, 0.587, 0.114)); }

float3 SampleNTSC(Sampler2D<float4> tex, float2 uv, float2 oneTexel) {
  float3 color = tex.Sample(uv).rgb;

  float3 yiq = mul(RGB_to_YIQ, color);

  float phase =
      (uv.x * scene.renderResolution.x * 0.5) + (float(scene.frame) * 0.5);
  float carrier = cos(phase * 6.28318);

  float3 neighbor = tex.Sample(uv + float2(oneTexel.x, 0.0)).rgb;
  float edge = luma(color) - luma(neighbor);

  if (abs(edge) > 0.1) {
    float i_mod = edge * carrier * NTSC_ARTIFACT_STR;
    float q_mod = edge * sin(phase * 6.28318) * NTSC_ARTIFACT_STR;

    float3 modRGB = mul(YIQ_to_RGB, float3(0.0, i_mod, q_mod));
    color += modRGB;
  }

  return color;
}

float3 SampleSignal(Sampler2D<float4> tex, float2 uv, float2 oneTexel,
                    float2 onePixel) {
  float3 sum = float3(0.0);
  float totalWeight = 0.0;

  for (int i = 0; i < 5; i++) {
    float2 blurStep = float2(offsets[i] * BLUR_WIDTH, 0.0) * onePixel;

    float shift = CONVERGENCE_OFFSET * oneTexel.x;

    float3 s;
    s.r = SampleNTSC(tex, uv + blurStep - float2(shift, 0.0), oneTexel).r;
    s.g = SampleNTSC(tex, uv + blurStep, oneTexel).g;
    s.b = SampleNTSC(tex, uv + blurStep + float2(shift, 0.0), oneTexel).b;

    sum += s * weights[i];
    totalWeight += weights[i];
  }
  return sum / totalWeight;
}

float3 SampleHalation(Sampler2D<float4> tex, float2 uv, float2 oneTexel) {
  float3 glow = float3(0.0);

  float samplesInner = 4.0;
  for (float i = 0.0; i < samplesInner; i++) {
    float angle = (i / samplesInner) * 6.28318;
    float2 finalOffset =
        float2(cos(angle), sin(angle)) * (HALATION_RADIUS * 0.35) * oneTexel;
    glow += tex.Sample(uv + finalOffset).rgb;
  }

  float samplesOuter = 8.0;
  for (float i = 0.0; i < samplesOuter; i++) {
    float angle = (i / samplesOuter) * 6.28318 + 0.785;
    float2 finalOffset =
        float2(cos(angle), sin(angle)) * HALATION_RADIUS * oneTexel;
    glow += tex.Sample(uv + finalOffset).rgb;
  }

  glow /= (samplesInner + samplesOuter);
  return max(glow - HALATION_THRESHOLD, 0.0);
}

public float3 ApplyCRT(float2 uv, float2 screenPos, Sampler2D<float4> tex) {
  float2 renderRes = scene.renderResolution;
  if (renderRes.x < 1.0)
    renderRes = float2(320.0, 240.0);
  float2 oneTexel = 1.0 / renderRes;
  float2 onePixel = 1.0 / scene.windowResolution;

  float3 color = SampleSignal(tex, uv, oneTexel, onePixel);

  float3 bloom = SampleHalation(tex, uv, oneTexel);
  color += bloom * HALATION_INTENSITY;

  bool isScanline = (int(screenPos.y) % 2) != 0;
  if (isScanline) {
    color *= SCANLINE_OPACITY;
  }

  float maskWave = sin(screenPos.x * 3.14159 * 1.5);
  float mask = (maskWave * 0.5 + 0.5) * MASK_INTENSITY + (1.0 - MASK_INTENSITY);
  color *= mask;

  float gray = luma(color);
  color = lerp(float3(gray), color, SATURATION);

  return color * BRIGHTNESS_BOOST;
}
