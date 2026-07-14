#include "loudness.h"
#include "config.h"
#include "eq.h"

// Fletcher-Munson loudness compensation
// Low volume = need more bass + treble to sound "full"
//
// Bass: low shelf 100Hz, max +8dB
// Treble: high shelf 10kHz, max +4dB
// Gain: max +6dB (2.0x)

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
    b.b0 = c[0]; b.b1 = c[1]; b.b2 = c[2]; b.a1 = c[3]; b.a2 = c[4];
    b.x1 = b.x2 = b.y1 = b.y2 = 0;
  }

  // type 2 = high shelf
  calc_biquad(2, 10000.0f, treble_db, 0.707f, (float)sample_rate, c);
  for (int ch = 0; ch < 2; ch++)
  {
    Biquad &b = ls.treble_shelf[ch];
    b.b0 = c[0]; b.b1 = c[1]; b.b2 = c[2]; b.a1 = c[3]; b.a2 = c[4];
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

  // auto on when volume < 0.4 (40%)
  if (vol < 0.4f)
  {
    // intensity scales linearly: vol=0.4→0%, vol=0.0→100%
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

static void biquad_process_stereo(Biquad *b, int16_t *s16, size_t frames)
{
  float b0 = b->b0, b1 = b->b1, b2 = b->b2;
  float a1 = b->a1, a2 = b->a2;
  float x1 = b->x1, x2 = b->x2, y1 = b->y1, y2 = b->y2;

  for (size_t i = 0; i < frames; i++)
  {
    float x0 = (float)s16[i];
    float acc = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x0; y2 = y1; y1 = acc;
    if (acc > 32767.0f) acc = 32767.0f;
    else if (acc < -32768.0f) acc = -32768.0f;
    s16[i] = (int16_t)acc;
  }

  b->x1 = x1; b->x2 = x2; b->y1 = y1; b->y2 = y2;
}

void loudness_process(LoudnessState &ls, int16_t *s16, size_t count)
{
  if (!ls.active || ls.intensity == 0)
    return;

  size_t frames = count / 2;

  for (int ch = 0; ch < 2; ch++)
  {
    int16_t tmp[512];
    for (size_t i = 0; i < frames; i++)
      tmp[i] = s16[i * 2 + ch];

    // bass shelf
    biquad_process_stereo(&ls.bass_shelf[ch], tmp, frames);
    // treble shelf
    biquad_process_stereo(&ls.treble_shelf[ch], tmp, frames);

    // gain: max +6dB (2.0x) scaled by intensity
    float pct = ls.intensity / 100.0f;
    float gain = 1.0f + pct; // 1.0x to 2.0x

    for (size_t i = 0; i < frames; i++)
    {
      float s = tmp[i] * gain;
      if (s > 32767.0f) s = 32767.0f;
      else if (s < -32768.0f) s = -32768.0f;
      tmp[i] = (int16_t)s;
    }

    for (size_t i = 0; i < frames; i++)
      s16[i * 2 + ch] = tmp[i];
  }
}
