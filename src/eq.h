#pragma once
#include <stdint.h>
#include <stddef.h>

struct Biquad
{
  float b0, b1, b2, a1, a2;
  float x1, x2, y1, y2;
};

struct EQ3Band
{
  Biquad band[2][3]; // [ch L/R][bass/mid/treble]
};

void calc_biquad(int type, float freq, float gain_db, float q, float fs, float *c);
void eq_init(EQ3Band &eq, uint32_t sample_rate);
void eq_set_band(EQ3Band &eq, int band, float freq, float gain_db, float q, uint32_t sample_rate);
void eq_process(EQ3Band &eq, int16_t *s16, size_t count);
