module Crt;
import depends.uniforms;

// --- CONFIGURATION ---
static const float SCANLINE_OPACITY = 0.4;
static const float BLUR_WIDTH = 1.0;
static const float CONVERGENCE_OFFSET = 0.5;

// --- BLOOM SETTINGS ---
static const float HALATION_INTENSITY = 0.6;
static const float HALATION_THRESHOLD = 0.2;

static const float NTSC_ARTIFACT_STR = 0.6;
static const float MASK_INTENSITY = 0.3;
static const float BRIGHTNESS_BOOST = 1.4;
static const float SATURATION = 1.1;

static const float weights[3] = { 0.28, 0.44, 0.28 };
static const float offsets[3] = { -1.0, 0.0, 1.0 };

float luma(float3 c) { return dot(c, float3(0.299, 0.587, 0.114)); }

float3 SampleHalation(Sampler2D<float4> texLinear, float2 uv) {
  float3 glow = texLinear.SampleLevel(uv, 2.0).rgb;
  return max(glow - HALATION_THRESHOLD, 0.0);
}

float3 SampleSignal(Sampler2D<float4> tex, float2 uv, float2 oneTexel,
                    float2 onePixel) {
  float shiftVal = CONVERGENCE_OFFSET * oneTexel.x;
  float2 shiftR = float2(-shiftVal, 0.0);
  float2 shiftB = float2(shiftVal, 0.0);

  float3 centerCol = tex.Sample(uv).rgb;
  float3 neighborCol = tex.Sample(uv + float2(oneTexel.x, 0.0)).rgb;
  float edge = luma(centerCol) - luma(neighborCol);
  bool hasEdge = abs(edge) > 0.05;

  float i_raw = hasEdge ? edge * NTSC_ARTIFACT_STR : 0.0;
  float q_raw = i_raw;

  float phaseScale = scene.renderResolution.x * 0.5;
  float phaseBase = float(scene.frame) * 0.5;

  float currentPhase =
      ((uv.x + offsets[0] * BLUR_WIDTH * onePixel.x) * phaseScale) + phaseBase;
  float phaseStep = BLUR_WIDTH * onePixel.x * phaseScale;

  float3 sum = float3(0.0);
  float totalWeight = 0.0;

  [unroll]
  for (int i = 0; i < 3; i++) {
    float2 tapUV = uv + float2(offsets[i] * BLUR_WIDTH, 0.0) * onePixel;
    float w = weights[i];

    float valR = tex.Sample(tapUV + shiftR).r;
    float valG = tex.Sample(tapUV).g;
    float valB = tex.Sample(tapUV + shiftB).b;
    float3 signal = float3(valR, valG, valB);

    if (hasEdge) {
      float sin_c, carrier_c;
      sincos(currentPhase * 6.28318, sin_c, carrier_c);

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
    currentPhase += phaseStep;
  }

  return sum / totalWeight;
}

public float3 ApplyCRT(float2 uv, float2 screenPos, Sampler2D<float4> tex,
                       Sampler2D<float4> texLinear) {
  float2 renderRes = scene.renderResolution;
  if (renderRes.x < 1.0)
    renderRes = float2(320.0, 240.0);

  float2 oneTexel = 1.0 / renderRes;
  float2 onePixel = 1.0 / scene.windowResolution;

  float3 color = SampleSignal(tex, uv, oneTexel, onePixel);

  if (HALATION_INTENSITY > 0.0) {
    float3 bloom = SampleHalation(texLinear, uv);
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
