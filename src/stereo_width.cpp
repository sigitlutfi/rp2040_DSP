#include "stereo_width.h"
#include "config.h"

volatile int16_t stereo_width_q8 = STEREO_WIDTH_Q8; // 628/256 = 2.45x

void stereo_width_process(int16_t *s16, size_t count, int16_t width_q8)
{
  size_t frames = count / 2;
  for (size_t i = 0; i < frames; i++)
  {
    int32_t L = s16[i * 2];
    int32_t R = s16[i * 2 + 1];
    int32_t M = (L + R) >> 1;
    int32_t S = (((L - R) * width_q8) >> 8);
    int32_t Lo = M + S;
    int32_t Ro = M - S;
    if (Lo > 32767)
      Lo = 32767;
    else if (Lo < -32768)
      Lo = -32768;
    if (Ro > 32767)
      Ro = 32767;
    else if (Ro < -32768)
      Ro = -32768;
    s16[i * 2] = (int16_t)Lo;
    s16[i * 2 + 1] = (int16_t)Ro;
  }
}
