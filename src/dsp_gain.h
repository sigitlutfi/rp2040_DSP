#pragma once
#include <stdint.h>
#include <stddef.h>

struct GainState
{
  int16_t gain_q8; // Q8, 1.0 = 256, 1.1 = 282
};

void gain_init(GainState &g, int16_t gain_q8);
void gain_process(GainState &g, int16_t *s16, size_t count);
