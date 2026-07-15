#include "eq.h"
#include "config.h"
#include <math.h>

void calc_biquad(int type, float freq, float gain_db, float q, float fs, float *c)
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
  c[0] = b0 * inv_a0;
  c[1] = b1 * inv_a0;
  c[2] = b2 * inv_a0;
  c[3] = a1 * inv_a0;
  c[4] = a2 * inv_a0;
}

void eq_set_band(EQ3Band &eq, int band, float freq, float gain_db, float q, uint32_t sample_rate)
{
  eq.db[band] = gain_db;
  int types[] = {0, 1, 2};
  float c[5];
  calc_biquad(types[band], freq, gain_db, q, (float)sample_rate, c);

  for (int ch = 0; ch < 2; ch++)
  {
    Biquad &b = eq.band[ch][band];
    b.b0 = (int16_t)(c[0] * 4096.0f);
    b.b1 = (int16_t)(c[1] * 4096.0f);
    b.b2 = (int16_t)(c[2] * 4096.0f);
    b.a1 = (int16_t)(c[3] * 4096.0f);
    b.a2 = (int16_t)(c[4] * 4096.0f);
    // Don't zero state variables - causes clicks/pops
  }
}

void eq_init(EQ3Band &eq, uint32_t sample_rate)
{
  eq_set_band(eq, 0, EQ_BASS_FREQ, EQ_BASS_DB, EQ_BASS_Q, sample_rate);
  eq_set_band(eq, 1, EQ_MID_FREQ, EQ_MID_DB, EQ_MID_Q, sample_rate);
  eq_set_band(eq, 2, EQ_TREBLE_FREQ, EQ_TREBLE_DB, EQ_TREBLE_Q, sample_rate);
}

// Process one biquad band on interleaved stereo, one channel
static void biquad_band_stereo(int16_t *s16, size_t frames, Biquad &b, int ch)
{
  int16_t cb0 = b.b0, cb1 = b.b1, cb2 = b.b2, ca1 = b.a1, ca2 = b.a2;
  int32_t x1 = b.x1, x2 = b.x2, y1 = b.y1, y2 = b.y2;

  for (size_t i = 0; i < frames; i++)
  {
    int32_t x0 = s16[i * 2 + ch];
    int32_t acc = (int32_t)cb0 * x0
                + (int32_t)cb1 * x1
                + (int32_t)cb2 * x2
                - (int32_t)ca1 * y1
                - (int32_t)ca2 * y2;
    x2 = x1;
    x1 = x0;
    int32_t y = acc >> 12;
    if (y > 32767) y = 32767;
    else if (y < -32768) y = -32768;
    y2 = y1;
    y1 = y;
    s16[i * 2 + ch] = (int16_t)y;
  }

  b.x1 = x1; b.x2 = x2;
  b.y1 = y1; b.y2 = y2;
}

// Process all 3 EQ bands per channel in single pass for better cache behavior
void eq_process(EQ3Band &eq, int16_t *s16, size_t count)
{
  size_t frames = count / 2;
  // Process left channel: all 3 bands sequentially
  for (int band = 0; band < 3; band++)
    biquad_band_stereo(s16, frames, eq.band[0][band], 0);
  // Process right channel: all 3 bands sequentially
  for (int band = 0; band < 3; band++)
    biquad_band_stereo(s16, frames, eq.band[1][band], 1);
}
