#include "exciter.h"
#include "config.h"

bool exciter_enabled = EXCITER_ENABLED_DEFAULT;
static int16_t exciter_mix_q8 = (int16_t)(EXCITER_MIX * 256.0f);

void exciter_process(int16_t *s16, size_t count)
{
  if (!exciter_enabled)
    return;

  int32_t mix = exciter_mix_q8;

  for (size_t i = 0; i < count; i++)
  {
    int32_t x = s16[i];
    int32_t x2 = (x * x) >> 15;   // x² in Q15
    int32_t x3 = (x2 * x) >> 15;  // x³ in Q15
    int32_t y = x - ((x3 * mix) >> 12); // x - mix * x³/16
    if (y > 32767) y = 32767;
    else if (y < -32768) y = -32768;
    s16[i] = (int16_t)y;
  }
}
