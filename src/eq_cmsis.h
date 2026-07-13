#pragma once
#include <stdint.h>
#include <stddef.h>
#include "arm_math.h"

struct BiquadCMSIS
{
  arm_biquad_casd_df1_inst_f32 inst;
  float coeffs[5];
  float state[4];
};

struct EQ3BandCMSIS
{
  BiquadCMSIS band[2][3]; // [ch][band]
};

void eq_cmsis_init(EQ3BandCMSIS &eq, uint32_t sample_rate);
void eq_cmsis_set_band(EQ3BandCMSIS &eq, int band, float freq, float gain_db, float q, uint32_t sample_rate);
void eq_cmsis_process(EQ3BandCMSIS &eq, int16_t *s16, size_t count);
