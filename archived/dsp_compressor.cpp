#include "dsp_compressor.h"

// Fixed-point Q8 dynamics compressor
// Zero float operations in inner loop

static const int16_t ONE_Q8 = 256;
static const int16_t MAX_SAMPLE = 32767;

void compressor_init(CompressorState &c, int16_t threshold_q8, int16_t ratio_q8)
{
  c.active = true;
  c.threshold = threshold_q8;
  c.ratio = ratio_q8;
  c.attack = 20;    // Q8: envelope attack
  c.release = 5;    // Q8: envelope release
  c.envelope = 0;
  c.gain = ONE_Q8;
}

void compressor_reset(CompressorState &c)
{
  c.envelope = 0;
  c.gain = ONE_Q8;
}

void compressor_process(CompressorState &c, int16_t *s16, size_t count)
{
  if (!c.active)
    return;

  int16_t thresh = c.threshold;
  int16_t ratio = c.ratio;
  int16_t att = c.attack;
  int16_t rel = c.release;
  int32_t env = c.envelope;
  int16_t gain = c.gain;

  // Gain smoothing: separate from envelope to avoid clicks
  // Slow attack (~200ms), very slow release (~800ms)
  const int16_t GAIN_ATTACK = 3;
  const int16_t GAIN_RELEASE = 1;

  for (size_t i = 0; i < count; i++)
  {
    int32_t sample = s16[i];

    // Absolute value with -32768 protection
    int16_t abs_sample = (int16_t)((sample < 0) ? ((sample == -32768) ? 32767 : -sample) : sample);

    // Envelope follower (Q8)
    int32_t diff = (int32_t)abs_sample - env;
    if (diff > 0)
      env += (diff * att) >> 8;
    else
      env += (diff * rel) >> 8;

    // Clamp envelope
    if (env < 0) env = 0;
    if (env > MAX_SAMPLE) env = MAX_SAMPLE;

    // Target gain with SOFT KNEE (gradual transition around threshold)
    int16_t target_gain;
    int16_t ratio_inv = (int16_t)((int32_t)ONE_Q8 * ONE_Q8 / ratio);

    // Soft knee: 2048 Q8 units = 8dB transition zone
    const int32_t KNEE = 2048;
    int32_t knee_low = thresh - KNEE / 2;
    int32_t knee_high = thresh + KNEE / 2;

    if (env <= knee_low || env <= 0)
    {
      // Below knee: no compression
      target_gain = ONE_Q8;
    }
    else if (env >= knee_high)
    {
      // Above knee: full compression
      target_gain = (int16_t)((int32_t)ratio_inv * thresh / env);
      if (target_gain > ONE_Q8) target_gain = ONE_Q8;
      if (target_gain < 0) target_gain = 0;
    }
    else
    {
      // Inside knee: interpolate unity → compressed
      int32_t pos = ((env - knee_low) << 8) / KNEE; // Q8: 0..256
      int16_t full_gain = (int16_t)((int32_t)ratio_inv * thresh / env);
      if (full_gain > ONE_Q8) full_gain = ONE_Q8;
      if (full_gain < 0) full_gain = 0;
      // Interpolate: 256 → full_gain based on position
      target_gain = (int16_t)(ONE_Q8 - (ONE_Q8 - full_gain) * pos / 256);
    }

    // Smooth gain VERY slowly to avoid clicks/rattling
    if (target_gain < gain)
      gain = gain - ((gain - target_gain) * GAIN_ATTACK >> 8);
    else
      gain = gain + ((target_gain - gain) * GAIN_RELEASE >> 8);

    // Apply gain
    s16[i] = (int16_t)(sample * gain >> 8);
  }

  c.envelope = env;
  c.gain = gain;
}
