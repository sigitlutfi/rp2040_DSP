#pragma once
#include <stdint.h>
#include <stddef.h>

struct NoiseGate
{
  int16_t threshold; // Q8, misal -40dB = 10 (256 * 10^(-40/20))
  int16_t attack;    // coefficient Q8
  int16_t release;   // coefficient Q8
  float envelope;
  bool gate_open;
};

void noise_gate_init(NoiseGate &g, int16_t threshold_q8);
void noise_gate_process(NoiseGate &g, int16_t *s16, size_t count);
