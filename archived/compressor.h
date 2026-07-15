#pragma once
#include <stdint.h>
#include <stddef.h>

struct CompressorState
{
  bool active;
  int16_t threshold; // Q8, misal -20dB = 82 (256 * 10^(-20/20))
  int16_t ratio;     // Q8, 4.0 = 1024, 2.0 = 512
  int16_t attack;    // coefficient Q8
  int16_t release;   // coefficient Q8
  float envelope;    // envelope follower
};

void compressor_init(CompressorState &c, int16_t threshold_q8, int16_t ratio_q8);
void compressor_process(CompressorState &c, int16_t *s16, size_t count);
