#pragma once
#include <stdint.h>
#include <stddef.h>

struct LimiterState
{
  int16_t threshold;  // peak level (Q8, default 32000)
  int16_t gain;       // current gain multiplier (Q8, 256 = unity)
  int16_t envelope;   // envelope follower (Q8, 0-256)
};

void limiter_init(LimiterState &l, int16_t threshold);
void limiter_process(LimiterState &l, int16_t *s16, size_t count);
