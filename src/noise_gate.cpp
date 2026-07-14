#include "noise_gate.h"
#include "config.h"

void noise_gate_init(NoiseGate &g, int16_t threshold_q8)
{
  g.active = true;
  g.threshold = threshold_q8;
  g.attack = GATE_ATTACK;
  g.release = GATE_RELEASE;
  g.envelope = 0;
  g.gate_open = false;
}

void noise_gate_process(NoiseGate &g, int16_t *s16, size_t count)
{
  if (!g.active)
    return;

  int32_t env = g.envelope;
  int32_t thresh = g.threshold;
  int32_t hyst = (thresh * 179) >> 8; // 70% for hysteresis
  int32_t att = g.attack;
  int32_t rel = g.release;

  for (size_t i = 0; i < count; i++)
  {
    int32_t sample = s16[i] < 0 ? -(int32_t)s16[i] : (int32_t)s16[i];

    // envelope follower (Q8 coefficients, integer math)
    if (sample > env)
      env += (att * (sample - env)) >> 8;
    else
      env += (rel * (sample - env)) >> 8;

    // gate logic with hysteresis
    if (env > thresh)
    {
      g.gate_open = true;
    }
    else if (env < hyst)
    {
      g.gate_open = false;
    }

    if (!g.gate_open)
    {
      s16[i] = 0;
    }
  }

  g.envelope = (int16_t)env;
}
