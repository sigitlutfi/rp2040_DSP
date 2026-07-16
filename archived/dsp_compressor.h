#pragma once
#include <stdint.h>
#include <stddef.h>

// Fixed-point Q8 dynamics compressor for M0+ (no FPU)
// All calculations use integer arithmetic

struct CompressorState
{
  bool active;
  int16_t threshold;  // Q8: level above which compression starts
  int16_t ratio;      // Q8: compression ratio (256=1:1, 512=2:1, 1024=4:1)
  int16_t attack;     // Q8: attack coefficient (higher = faster)
  int16_t release;    // Q8: release coefficient (higher = faster)
  int32_t envelope;   // Q8: envelope follower (32-bit for precision)
  int16_t gain;       // Q8: current gain multiplier
};

void compressor_init(CompressorState &c, int16_t threshold_q8, int16_t ratio_q8);
void compressor_reset(CompressorState &c);
void compressor_process(CompressorState &c, int16_t *s16, size_t count);
