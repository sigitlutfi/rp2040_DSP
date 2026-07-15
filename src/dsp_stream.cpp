#include "dsp_stream.h"

// ---------- EQ ----------
volatile bool eq_needs_reinit = false;
volatile bool eq_pending_update = false;
volatile float eq_pending_db[3] = {0.0f, 0.0f, 0.0f};
EQ3Band eq_state;

// ---------- Stereo Width ----------
extern volatile int16_t stereo_width_q8;

// ---------- Compressor ----------
CompressorState comp_state;

// ---------- Gain ----------
GainState gain_state;

// ---------- Limiter ----------
LimiterState limiter_state;

// ---------- Noise Gate ----------
NoiseGate gate_state;

// ---------- Loudness ----------
LoudnessState loudness_state;

// ---------- Reverb ----------
ReverbState reverb_state;

// ---------- Per-module enable flags ----------
bool eq_enabled = EQ_ENABLED_DEFAULT;
bool width_enabled = WIDTH_ENABLED_DEFAULT;

// ---------- Reset flag ----------
volatile bool dsp_needs_reset = false;

size_t DSPStream::readBytes(uint8_t *buffer, size_t length)
{
  size_t total = 0;
  while (total < length) {
    int b = read();
    if (b < 0) break;
    buffer[total++] = (uint8_t)b;
  }
  return total;
}
