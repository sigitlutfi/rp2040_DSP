#include "exciter.h"
#include "config.h"

static float exciter_mix = EXCITER_MIX;

void exciter_process(int16_t *s16, size_t count)
{
  float mix = exciter_mix;
  for (size_t i = 0; i < count; i++)
  {
    float x = (float)s16[i] / 32768.0f;    // normalize to [-1, 1]
    float wet = x - (x * x * x) * 0.0625f; // x - x³/16
    float y = x + (wet - x) * mix;         // dry + wet * mix
    if (y > 1.0f)
      y = 1.0f;
    else if (y < -1.0f)
      y = -1.0f;
    s16[i] = (int16_t)(y * 32768.0f);
  }
}
