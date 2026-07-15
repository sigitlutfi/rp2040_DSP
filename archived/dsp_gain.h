#pragma once
#include <stdint.h>
#include <stddef.h>

struct GainState
{
  int16_t gain_q8;      // user gain (Q8, set via serial)
  int16_t volume_q8;    // USB volume (Q8, 0=silence, 256=unity)
  int16_t combined_q8;  // precomputed (gain_q8 * volume_q8) >> 8
};

void gain_init(GainState &g, int16_t gain_q8);
void gain_set_gain(GainState &g, int16_t gain_q8);
void gain_set_volume(GainState &g, int16_t volume_q8);
void gain_process(GainState &g, int16_t *s16, size_t count);
