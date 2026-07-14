#include "dsp_limiter.h"

void limiter_init(LimiterState &l, int16_t threshold)
{
  l.threshold = threshold;
}

void limiter_process(LimiterState &l, int16_t *s16, size_t count)
{
  int16_t lt = l.threshold;
  for (size_t i = 0; i < count; i++)
  {
    int32_t s = s16[i];
    if (s > lt)
      s = lt;
    else if (s < -lt)
      s = -lt;
    s16[i] = (int16_t)s;
  }
}
