#include "dsp_limiter.h"

// Feed-forward limiter with attack/release envelope
// Attack: fast (~2ms) to catch peaks
// Release: slow (~50ms) to avoid pumping

static const int16_t ATTACK_RATE = 12;   // Q8: fast attack (~2ms at 44100Hz)
static const int16_t RELEASE_RATE = 5;   // Q8: slow release (~50ms)
static const int16_t ONE_Q8 = 256;

void limiter_init(LimiterState &l, int16_t threshold)
{
  l.threshold = threshold;
  l.gain = ONE_Q8;
  l.envelope = ONE_Q8;
}

void limiter_reset(LimiterState &l)
{
  l.gain = ONE_Q8;
  l.envelope = 0;
}

void limiter_process(LimiterState &l, int16_t *s16, size_t count)
{
  int16_t thresh = l.threshold;
  int16_t env = l.envelope;
  int16_t gain = l.gain;

  for (size_t i = 0; i < count; i++)
  {
    // Detect peak (magnitude)
    int32_t sample = s16[i];
    int16_t abs_sample = (int16_t)((sample < 0) ? ((sample == -32768) ? 32767 : -sample) : sample);

    // Peak follower (envelope)
    if (abs_sample > env)
      env = abs_sample;  // instant attack
    else
      env = env - (env - abs_sample) * RELEASE_RATE / ONE_Q8;

    // Calculate required gain reduction
    int16_t new_gain;
    if (env > thresh && env > 0)
    {
      // gain needed: threshold / envelope, in Q8
      new_gain = (int16_t)((int32_t)thresh * ONE_Q8 / env);
    }
    else
    {
      new_gain = ONE_Q8; // unity
    }

    // Smooth gain changes (attack/release)
    if (new_gain < gain)
      gain = gain - (gain - new_gain) * ATTACK_RATE / ONE_Q8;  // fast attack
    else
      gain = gain + (new_gain - gain) * RELEASE_RATE / ONE_Q8;  // slow release

    // Apply gain
    s16[i] = (int16_t)((int32_t)sample * gain >> 8);
  }

  l.envelope = env;
  l.gain = gain;
}
