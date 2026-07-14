#pragma once
#include <stdint.h>
#include <stddef.h>

struct LimiterState
{
  int16_t threshold; // Q8, misal 32000
};

void limiter_init(LimiterState &l, int16_t threshold);
void limiter_process(LimiterState &l, int16_t *s16, size_t count);
