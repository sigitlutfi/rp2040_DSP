#include "loudness.h"
#include "config.h"
#include "eq.h"

// Fletcher-Munson loudness compensation
// Low volume = need more bass + treble to sound "full"
//
// Bass: low shelf 100Hz, max +8dB
// Treble: high shelf 10kHz, max +4dB
// Gain: max +6dB (2.0x) via Q8 integer

static void loudness_recalc(LoudnessState &ls, uint32_t sample_rate)
{
  float pct = ls.intensity / 100.0f;
  float bass_db = 8.0f * pct;
  float treble_db = 4.0f * pct;

  float c[5];

  // type 0 = low shelf
  calc_biquad(0, 100.0f, bass_db, 0.707f, (float)sample_rate, c);
  for (int ch = 0; ch < 2; ch++)
  {
    Biquad &b = ls.bass_shelf[ch];
    b.b0 = (int16_t)(c[0] * 4096.0f);
    b.b1 = (int16_t)(c[1] * 4096.0f);
    b.b2 = (int16_t)(c[2] * 4096.0f);
    b.a1 = (int16_t)(c[3] * 4096.0f);
    b.a2 = (int16_t)(c[4] * 4096.0f);
    b.x1 = b.x2 = b.y1 = b.y2 = 0;
  }

  // type 2 = high shelf
  calc_biquad(2, 10000.0f, treble_db, 0.707f, (float)sample_rate, c);
  for (int ch = 0; ch < 2; ch++)
  {
    Biquad &b = ls.treble_shelf[ch];
    b.b0 = (int16_t)(c[0] * 4096.0f);
    b.b1 = (int16_t)(c[1] * 4096.0f);
    b.b2 = (int16_t)(c[2] * 4096.0f);
    b.a1 = (int16_t)(c[3] * 4096.0f);
    b.a2 = (int16_t)(c[4] * 4096.0f);
    b.x1 = b.x2 = b.y1 = b.y2 = 0;
  }
}

void loudness_init(LoudnessState &ls, uint32_t sample_rate)
{
  ls.usb_volume = 1.0f;
  ls.active = LOUDNESS_ACTIVE_DEFAULT;
  ls.auto_mode = LOUDNESS_AUTO_DEFAULT;
  ls.intensity = 0;
  loudness_recalc(ls, sample_rate);
}

void loudness_set_usb_volume(LoudnessState &ls, float vol, uint32_t sample_rate)
{
  ls.usb_volume = vol;
  if (!ls.auto_mode)
    return;

  if (vol < 0.4f)
  {
    ls.intensity = (uint8_t)((0.4f - vol) / 0.4f * 100.0f);
    ls.active = true;
  }
  else
  {
    ls.intensity = 0;
    ls.active = false;
  }
  loudness_recalc(ls, sample_rate);
}

void loudness_set_manual(LoudnessState &ls, bool on, uint8_t intensity, uint32_t sample_rate)
{
  ls.auto_mode = false;
  ls.active = on;
  ls.intensity = on ? intensity : 0;
  loudness_recalc(ls, sample_rate);
}

// Q12 biquad on one channel, stride-2 access (same as eq.cpp)
static void biquad_loudness_stereo(int16_t *s16, size_t frames, Biquad &b, int ch)
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

void loudness_process(LoudnessState &ls, int16_t *s16, size_t count)
{
  if (!ls.active || ls.intensity == 0)
    return;

  size_t frames = count / 2;
  int32_t gain_q8 = 256 + (int32_t)ls.intensity * 256 / 100; // 1.0x to 2.0x

  for (int ch = 0; ch < 2; ch++)
  {
    biquad_loudness_stereo(s16, frames, ls.bass_shelf[ch], ch);
    biquad_loudness_stereo(s16, frames, ls.treble_shelf[ch], ch);

    for (size_t i = 0; i < frames; i++)
    {
      int32_t s = ((int32_t)s16[i * 2 + ch] * gain_q8) >> 8;
      if (s > 32767) s = 32767;
      else if (s < -32768) s = -32768;
      s16[i * 2 + ch] = (int16_t)s;
    }
  }
}
