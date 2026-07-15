#include "dither.h"

static uint32_t rng_state = 12345;

static int16_t tpdf_dither()
{
  // xorshift32
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  if (rng_state == 0) rng_state = 12345;

  // Two random bits → triangular ±1 LSB
  int32_t r1 = rng_state & 1;
  int32_t r2 = (rng_state >> 1) & 1;
  return (int16_t)(r1 + r2 - 1); // -1, 0, or +1
}

void dither_process(int16_t *s16, size_t count)
{
  for (size_t i = 0; i < count; i++)
  {
    int32_t s = s16[i] + tpdf_dither();
    if (s > 32767)
      s = 32767;
    else if (s < -32768)
      s = -32768;
    s16[i] = (int16_t)s;
  }
}
