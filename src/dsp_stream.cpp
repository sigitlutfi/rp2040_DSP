#include "dsp_stream.h"

// ---------- EQ ----------
volatile bool eq_needs_reinit = false;
EQ3Band eq_state;

// ---------- Stereo Width ----------
extern volatile int16_t stereo_width_q8;

// ---------- Integer-only Effects (Q8 fixed-point) ----------
volatile int16_t gain_q8 = 282; // 1.1x
volatile int16_t limit_threshold = 32000;

size_t DSPStream::readBytes(uint8_t *buffer, size_t length)
{
  size_t n = _source.readBytes(buffer, length);
  if (n > 0)
  {
    if (eq_needs_reinit)
    {
      eq_init(eq_state, current_sample_rate);
      eq_needs_reinit = false;
    }

    eq_process(eq_state, (int16_t *)buffer, n / 2);

    stereo_width_process((int16_t *)buffer, n / 2, stereo_width_q8);

    exciter_process((int16_t *)buffer, n / 2);

    int16_t *s16 = (int16_t *)buffer;
    size_t count = n / 2;
    int16_t g = gain_q8;
    int16_t lt = limit_threshold;
    for (size_t i = 0; i < count; i++)
    {
      int32_t s = ((int32_t)s16[i] * g) >> 8;
      if (s > lt)
        s = lt;
      else if (s < -lt)
        s = -lt;
      s16[i] = (int16_t)s;
    }
  }
  return n;
}
