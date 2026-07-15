#include "dsp_gain.h"

static inline void recombine(GainState &g)
{
  g.combined_q8 = ((int32_t)g.gain_q8 * g.volume_q8) >> 8;
}

void gain_init(GainState &g, int16_t gain_q8)
{
  g.gain_q8 = gain_q8;
  g.volume_q8 = 256;
  g.combined_q8 = gain_q8;
}

void gain_set_gain(GainState &g, int16_t gain_q8)
{
  g.gain_q8 = gain_q8;
  recombine(g);
}

void gain_set_volume(GainState &g, int16_t volume_q8)
{
  g.volume_q8 = volume_q8;
  recombine(g);
}

void gain_process(GainState &g, int16_t *s16, size_t count)
{
  int16_t g_val = g.combined_q8;
  for (size_t i = 0; i < count; i++)
  {
    int32_t s = ((int32_t)s16[i] * g_val) >> 8;
    if (s > 32767)
      s = 32767;
    else if (s < -32768)
      s = -32768;
    s16[i] = (int16_t)s;
  }
}
