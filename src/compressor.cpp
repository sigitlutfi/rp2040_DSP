#include "compressor.h"
#include "config.h"
#include <math.h>

void compressor_init(CompressorState &c, int16_t threshold_q8, int16_t ratio_q8)
{
  c.threshold = threshold_q8;
  c.ratio = ratio_q8;
  c.attack = COMPRESSOR_ATTACK;
  c.release = COMPRESSOR_RELEASE;
  c.envelope = 0.0f;
}

void compressor_process(CompressorState &c, int16_t *s16, size_t count)
{
  float env = c.envelope;
  float thresh = (float)c.threshold;
  float ratio_inv = 256.0f / (float)c.ratio; // 1/ratio in Q8

  for (size_t i = 0; i < count; i++)
  {
    float sample = fabsf((float)s16[i]);

    // envelope follower
    if (sample > env)
      env += c.attack * (sample - env) / 256.0f;
    else
      env += c.release * (sample - env) / 256.0f;

    // gain reduction
    if (env > thresh)
    {
      float over = env - thresh;
      float compressed_over = over * ratio_inv / 256.0f;
      float gain = (thresh + compressed_over) / env;
      s16[i] = (int16_t)(s16[i] * gain);
    }
  }

  c.envelope = env;
}
