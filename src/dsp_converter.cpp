#include "dsp_converter.h"
#include "vu.h"

extern volatile float eq_pending_db[3];

volatile uint32_t dsp_cpu_us = 0;
volatile uint16_t dsp_block_bytes = 0;

size_t DSPConverter::convert(uint8_t *src, size_t byte_count)
{
  if (byte_count == 0) return 0;

  unsigned long t0 = micros();

  int16_t *s16 = (int16_t *)src;
  size_t count = byte_count / sizeof(int16_t);

  if (dsp_needs_reset)
  {
    eq_init(eq_state, current_sample_rate);
    eq_reset_state(eq_state);
    limiter_reset(limiter_state);
    vu_peak_l = 0;
    vu_peak_r = 0;
    eq_needs_reinit = false;
    dsp_needs_reset = false;
  }

  if (eq_needs_reinit)
  {
    float freqs[] = {EQ_BASS_FREQ, EQ_MID_FREQ, EQ_TREBLE_FREQ};
    float qs[] = {EQ_BASS_Q, EQ_MID_Q, EQ_TREBLE_Q};
    for (int b = 0; b < 3; b++)
      eq_set_band(eq_state, b, freqs[b], eq_pending_db[b], qs[b], current_sample_rate);
    eq_needs_reinit = false;
  }

  if (eq_enabled)
    eq_process(eq_state, s16, count);

  if (width_enabled)
    stereo_width_process(s16, count, stereo_width_q8);

  int16_t vol = usb_volume_q8;
  if (vol < 256) {
    for (size_t i = 0; i < count; i++)
      s16[i] = (int16_t)((int32_t)s16[i] * vol >> 8);
  }

  limiter_process(limiter_state, s16, count);

  // VU peak (L/R interleaved)
  int16_t peak_l = 0, peak_r = 0;
  for (size_t i = 0; i < count; i += 2) {
    int16_t l = s16[i];
    int16_t r = s16[i + 1];
    if (l < 0) l = -l;
    if (r < 0) r = -r;
    if (l > peak_l) peak_l = l;
    if (r > peak_r) peak_r = r;
  }
  vu_peak_l = peak_l;
  vu_peak_r = peak_r;

  dsp_cpu_us = micros() - t0;
  dsp_block_bytes = byte_count;

  return byte_count;
}
