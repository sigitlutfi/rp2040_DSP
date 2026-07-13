#include "eq_cmsis.h"
#include <math.h>

static void calc_coeffs_cmsis(int type, float freq, float gain_db, float q, float fs, float *c)
{
  float A = powf(10.0f, gain_db / 40.0f);
  float w0 = 2.0f * 3.14159265f * freq / fs;
  float alpha = sinf(w0) / (2.0f * q);
  float cw = cosf(w0);

  float b0, b1, b2, a0, a1, a2;

  if (type == 0) // low shelf
  {
    float sqA = sqrtf(A);
    b0 = A * ((A + 1) - (A - 1) * cw + 2 * sqA * alpha);
    b1 = 2 * A * ((A - 1) - (A + 1) * cw);
    b2 = A * ((A + 1) - (A - 1) * cw - 2 * sqA * alpha);
    a0 = (A + 1) + (A - 1) * cw + 2 * sqA * alpha;
    a1 = -2 * ((A - 1) + (A + 1) * cw);
    a2 = (A + 1) + (A - 1) * cw - 2 * sqA * alpha;
  }
  else if (type == 1) // peaking
  {
    b0 = 1 + alpha * A;
    b1 = -2 * cw;
    b2 = 1 - alpha * A;
    a0 = 1 + alpha / A;
    a1 = -2 * cw;
    a2 = 1 - alpha / A;
  }
  else // high shelf
  {
    float sqA = sqrtf(A);
    b0 = A * ((A + 1) + (A - 1) * cw + 2 * sqA * alpha);
    b1 = -2 * A * ((A - 1) + (A + 1) * cw);
    b2 = A * ((A + 1) + (A - 1) * cw - 2 * sqA * alpha);
    a0 = (A + 1) - (A - 1) * cw + 2 * sqA * alpha;
    a1 = 2 * ((A - 1) - (A + 1) * cw);
    a2 = (A + 1) - (A - 1) * cw - 2 * sqA * alpha;
  }

  float inv_a0 = 1.0f / a0;
  // CMSIS-DSP order: b0, b1, b2, a1, a2 (a0 is normalized to 1)
  c[0] = b0 * inv_a0;
  c[1] = b1 * inv_a0;
  c[2] = b2 * inv_a0;
  c[3] = a1 * inv_a0;
  c[4] = a2 * inv_a0;
}

void eq_cmsis_set_band(EQ3BandCMSIS &eq, int band, float freq, float gain_db, float q, uint32_t sample_rate)
{
  int types[] = {0, 1, 2};
  float c[5];
  calc_coeffs_cmsis(types[band], freq, gain_db, q, (float)sample_rate, c);

  for (int ch = 0; ch < 2; ch++)
  {
    BiquadCMSIS &b = eq.band[ch][band];
    for (int i = 0; i < 5; i++)
      b.coeffs[i] = c[i];
    for (int i = 0; i < 4; i++)
      b.state[i] = 0.0f;

    arm_biquad_cascade_df1_init_f32(&b.inst, 1, b.coeffs, b.state);
  }
}

void eq_cmsis_init(EQ3BandCMSIS &eq, uint32_t sample_rate)
{
  eq_cmsis_set_band(eq, 0, 200.0f, 0.0f, 0.707f, sample_rate);
  eq_cmsis_set_band(eq, 1, 1000.0f, 0.0f, 1.0f, sample_rate);
  eq_cmsis_set_band(eq, 2, 8000.0f, 0.0f, 0.707f, sample_rate);
}

void eq_cmsis_process(EQ3BandCMSIS &eq, int16_t *s16, size_t count)
{
  size_t frames = count / 2;
  float tmp[512];

  for (int ch = 0; ch < 2; ch++)
  {
    // int16 → float
    for (size_t i = 0; i < frames; i++)
      tmp[i] = (float)s16[i * 2 + ch];

    // 3 bands berturut-turut
    for (int band = 0; band < 3; band++)
    {
      float out[512];
      arm_biquad_cascade_df1_f32(&eq.band[ch][band].inst, tmp, out, frames);

      // copy balik ke tmp
      for (size_t i = 0; i < frames; i++)
        tmp[i] = out[i];
    }

    // float → int16, clamp
    for (size_t i = 0; i < frames; i++)
    {
      float v = tmp[i];
      if (v > 32767.0f)
        v = 32767.0f;
      else if (v < -32768.0f)
        v = -32768.0f;
      s16[i * 2 + ch] = (int16_t)v;
    }
  }
}
