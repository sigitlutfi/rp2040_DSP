#include "noise_gate.h"
#include "config.h"
#include <math.h>

void noise_gate_init(NoiseGate &g, int16_t threshold_q8)
{
  g.threshold = threshold_q8;
  g.attack = GATE_ATTACK;
  g.release = GATE_RELEASE;
  g.envelope = 0.0f;
  g.gate_open = false;
}

void noise_gate_process(NoiseGate &g, int16_t *s16, size_t count)
{
  float env = g.envelope;
  float thresh = (float)g.threshold;

  for (size_t i = 0; i < count; i++)
  {
    float sample = fabsf((float)s16[i]);

    // envelope follower
    if (sample > env)
      env += g.attack * (sample - env) / 256.0f;
    else
      env += g.release * (sample - env) / 256.0f;

    // gate logic
    if (env > thresh)
    {
      g.gate_open = true;
    }
    else if (env < thresh * 0.7f) // hysteresis 70%
    {
      g.gate_open = false;
    }

    if (!g.gate_open)
    {
      s16[i] = 0;
    }
  }

  g.envelope = env;
}
