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
  int types[] = {0, 1, 2};
  float c[5];
  calc_biquad(types[band], freq, gain_db, q, (float)sample_rate, c);

  for (int ch = 0; ch < 2; ch++)
  {
    Biquad &b = eq.band[ch][band];
    b.b0 = c[0];
    b.b1 = c[1];
    b.b2 = c[2];
    b.a1 = c[3];
    b.a2 = c[4];
    b.x1 = b.x2 = b.y1 = b.y2 = 0;
  }
}

void eq_init(EQ3Band &eq, uint32_t sample_rate)
{
  eq_set_band(eq, 0, EQ_BASS_FREQ, EQ_BASS_DB, EQ_BASS_Q, sample_rate);
  eq_set_band(eq, 1, EQ_MID_FREQ, EQ_MID_DB, EQ_MID_Q, sample_rate);
  eq_set_band(eq, 2, EQ_TREBLE_FREQ, EQ_TREBLE_DB, EQ_TREBLE_Q, sample_rate);
}

static void biquad_process(Biquad &b, int16_t *data, size_t n)
{
  float b0 = b.b0, b1 = b.b1, b2 = b.b2;
  float a1 = b.a1, a2 = b.a2;
  float x1 = b.x1, x2 = b.x2, y1 = b.y1, y2 = b.y2;

  for (size_t i = 0; i < n; i++)
  {
    float x0 = (float)data[i];
    float acc = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = x0;
    y2 = y1;
    y1 = acc;
    if (acc > 32767.0f)
      acc = 32767.0f;
    else if (acc < -32768.0f)
      acc = -32768.0f;
    data[i] = (int16_t)acc;
  }

  b.x1 = x1;
  b.x2 = x2;
  b.y1 = y1;
  b.y2 = y2;
}

void eq_process(EQ3Band &eq, int16_t *s16, size_t count)
{
  size_t frames = count / 2;

  for (int ch = 0; ch < 2; ch++)
  {
    for (int band = 0; band < 3; band++)
    {
      // extract channel, process, write back
      int16_t tmp[512];
      for (size_t i = 0; i < frames; i++)
        tmp[i] = s16[i * 2 + ch];

      biquad_process(eq.band[ch][band], tmp, frames);

      for (size_t i = 0; i < frames; i++)
        s16[i * 2 + ch] = tmp[i];
    }
  }
}
