#include "dsp_converter.h"

size_t DSPConverter::convert(uint8_t *src, size_t byte_count)
{
  if (byte_count == 0) return 0;

  int16_t *s16 = (int16_t *)src;
  size_t count = byte_count / sizeof(int16_t);

  if (dsp_needs_reset)
  {
    eq_init(eq_state, current_sample_rate);
    eq_needs_reinit = false;
    dsp_needs_reset = false;
  }

  if (eq_needs_reinit)
  {
    eq_init(eq_state, current_sample_rate);
    eq_needs_reinit = false;
  }

  if (eq_enabled)
    eq_process(eq_state, s16, count);

  if (width_enabled)
    stereo_width_process(s16, count, stereo_width_q8);

  // Apply USB host volume (integer Q8 multiply)
  int16_t vol = usb_volume_q8;
  if (vol < 256) {
    for (size_t i = 0; i < count; i++)
      s16[i] = (int16_t)((int32_t)s16[i] * vol >> 8);
  }

  limiter_process(limiter_state, s16, count);

  return byte_count;
}

