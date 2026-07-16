#pragma once
#include <stdint.h>
#include <stddef.h>

// Fixed-point Q12 biquad for M0+ (no FPU)
// Coefficients stored as Q12 int16_t (range ±8.0)
// State variables as int32_t for precision

struct Biquad
{
  int16_t b0, b1, b2, a1, a2; // Q12 coefficients
  int32_t x1, x2, y1, y2;     // state (Q15 scale)
};

struct EQ3Band
{
  Biquad band[2][3]; // [ch L/R][bass/mid/treble]
  float db[3];       // runtime dB values for GET/STATUS
};

void calc_biquad(int type, float freq, float gain_db, float q, float fs, float *c);
void eq_set_band(EQ3Band &eq, int band, float freq, float gain_db, float q, uint32_t sample_rate);
void eq_init(EQ3Band &eq, uint32_t sample_rate);
void eq_reset_state(EQ3Band &eq);
void eq_process(EQ3Band &eq, int16_t *s16, size_t count);
