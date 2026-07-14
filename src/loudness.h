#pragma once
#include <stdint.h>
#include <stddef.h>
#include "eq.h"

struct LoudnessState
{
  Biquad bass_shelf[2];   // [ch] low shelf ~100Hz
  Biquad treble_shelf[2]; // [ch] high shelf ~10kHz
  float usb_volume;       // 0.0-1.0 from PC
  bool active;            // current on/off
  bool auto_mode;         // auto (volume-based) vs manual
  uint8_t intensity;      // 0-100 manual strength
};

void loudness_init(LoudnessState &ls, uint32_t sample_rate);
void loudness_set_usb_volume(LoudnessState &ls, float vol, uint32_t sample_rate);
void loudness_set_manual(LoudnessState &ls, bool on, uint8_t intensity, uint32_t sample_rate);
void loudness_process(LoudnessState &ls, int16_t *s16, size_t count);
