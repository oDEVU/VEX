module Crt;
import depends.uniforms;

// --- CONFIGURATION ---
static const float SCANLINE_OPACITY = 0.4;
static const float BLUR_WIDTH = 1.0;
static const float CONVERGENCE_OFFSET = 0.5;

// --- BLOOM SETTINGS ---
static const float HALATION_INTENSITY = 0.5;
static const float HALATION_THRESHOLD = 0.1;

static const float NTSC_ARTIFACT_STR = 0.6;
static const float MASK_INTENSITY = 0.3;
static const float BRIGHTNESS_BOOST = 1.4;
static const float SATURATION = 1.1;

static const float weights[3] = { 0.28, 0.44, 0.28 };
static const float offsets[3] = { -1.0, 0.0, 1.0 };

static const float2 halation_offsets[12] = {
  float2(0.35, 0.0),      float2(0.0, 0.35),     float2(-0.35, 0.0),
  float2(0.0, -0.35),     float2(0.707, 0.707),  float2(-0.707, 0.707),
  float2(-0.707, -0.707), float2(0.707, -0.707), float2(1.0, 0.0),
  float2(0.0, 1.0),       float2(-1.0, 0.0),     float2(0.0, -1.0)
};

float luma(float3 c) { return dot(c, float3(0.299, 0.587, 0.114)); }

float3 SampleSignal(Sampler2D<float4> tex, float2 uv, float2 oneTexel,
                    float2 onePixel) {
  float3 sum = float3(0.0);
  float totalWeight = 0.0;

  float shiftVal = CONVERGENCE_OFFSET * oneTexel.x;
  float2 shiftR = float2(-shiftVal, 0.0);
  float2 shiftB = float2(shiftVal, 0.0);

  float3 centerCol = tex.Sample(uv).rgb;
  float3 neighborCol = tex.Sample(uv + float2(oneTexel.x, 0.0)).rgb;
  float edge = luma(centerCol) - luma(neighborCol);
  bool hasEdge = abs(edge) > 0.05;

  float phaseBase = (float(scene.frame) * 0.5);
  float phaseScale = scene.renderResolution.x * 0.5;

  float i_raw = 0.0;
  float q_raw = 0.0;
  if (hasEdge) {
    i_raw = edge * NTSC_ARTIFACT_STR;
    q_raw = i_raw;
  }

  [unroll]
  for (int i = 0; i < 3; i++) {
    float2 tapUV = uv + float2(offsets[i] * BLUR_WIDTH, 0.0) * onePixel;
    float w = weights[i];

    float valR = tex.Sample(tapUV + shiftR).r;
    float valG = tex.Sample(tapUV).g;
    float valB = tex.Sample(tapUV + shiftB).b;

    float3 signal = float3(valR, valG, valB);

    if (hasEdge) {
      float phase = (tapUV.x * phaseScale) + phaseBase;

      float carrier_c = cos(phase * 6.28318);
      float sin_c = sin(phase * 6.28318);

      float i_r = i_raw * sin_c;
      float q_r = q_raw * -carrier_c;
      signal.r += (0.956 * i_r + 0.621 * q_r);

      float i_g = i_raw * carrier_c;
      float q_g = q_raw * sin_c;
      signal.g += (-0.272 * i_g - 0.647 * q_g);

      float i_b = i_raw * -sin_c;
      float q_b = q_raw * carrier_c;
      signal.b += (-1.106 * i_b + 1.703 * q_b);
    }

    sum += signal * w;
    totalWeight += w;
  }

  return sum / totalWeight;
}

float3 SampleHalation(Sampler2D<float4> tex, float2 uv, float2 oneTexel) {
  float3 glow = float3(0.0);
  float radius = 2.5;

  [unroll]
  for (int i = 0; i < 12; i++) {
    float2 offset = halation_offsets[i] * radius * oneTexel;
    glow += tex.Sample(uv + offset).rgb;
  }

  return max((glow / 12.0) - HALATION_THRESHOLD, 0.0);
}

public float3 ApplyCRT(float2 uv, float2 screenPos, Sampler2D<float4> tex) {
  float2 renderRes = scene.renderResolution;
  if (renderRes.x < 1.0)
    renderRes = float2(320.0, 240.0);

  float2 oneTexel = 1.0 / renderRes;
  float2 onePixel = 1.0 / scene.windowResolution;

  float3 color = SampleSignal(tex, uv, oneTexel, onePixel);

  if (HALATION_INTENSITY > 0.0) {
    float3 bloom = SampleHalation(tex, uv, oneTexel);
    color += bloom * HALATION_INTENSITY;
  }

  bool isScanline = (int(screenPos.y) % 2) != 0;
  color *= lerp(1.0, SCANLINE_OPACITY, float(isScanline));

  float maskWave = sin(screenPos.x * 3.14159 * 1.5);
  float mask = (maskWave * 0.5 + 0.5) * MASK_INTENSITY + (1.0 - MASK_INTENSITY);
  color *= mask;

  float gray = luma(color);
  color = lerp(float3(gray), color, SATURATION);

  return color * BRIGHTNESS_BOOST;
}
