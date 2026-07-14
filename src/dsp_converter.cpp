#include "dsp_converter.h"

size_t DSPConverter::convert(uint8_t *src, size_t byte_count)
{
  if (byte_count == 0) return 0;

  int16_t *s16 = (int16_t *)src;
  size_t count = byte_count / sizeof(int16_t);

  // Reset DSP states on sample rate change or stream restart
  if (dsp_needs_reset)
  {
    eq_init(eq_state, current_sample_rate);
    eq_needs_reinit = false;
    limiter_init(limiter_state, LIMITER_THRESHOLD);
    gain_init(gain_state, 256);
    dsp_needs_reset = false;
  }

  if (eq_needs_reinit)
  {
    eq_init(eq_state, current_sample_rate);
    eq_needs_reinit = false;
  }

  // Gain (Q8 int) — 1.0x = unity
  gain_process(gain_state, s16, count);

  // EQ (Q12 fixed-point biquad)
  if (eq_enabled)
    eq_process(eq_state, s16, count);

  // Stereo Width (Q8 int)
  if (width_enabled)
    stereo_width_process(s16, count, stereo_width_q8);

  // Limiter
  limiter_process(limiter_state, s16, count);

  // TPDF Dither
  dither_process(s16, count);

  return byte_count;
}
