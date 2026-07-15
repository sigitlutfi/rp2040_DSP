#include "dsp_stream.h"

// ---------- EQ ----------
volatile bool eq_needs_reinit = false;
volatile bool eq_pending_update = false;
volatile float eq_pending_db[3] = {EQ_BASS_DB, EQ_MID_DB, EQ_TREBLE_DB};
EQ3Band eq_state;

// ---------- Stereo Width ----------
extern volatile int16_t stereo_width_q8;


// ---------- Limiter ----------
LimiterState limiter_state;



// ---------- Per-module enable flags ----------
volatile bool eq_enabled = EQ_ENABLED_DEFAULT;
volatile bool width_enabled = WIDTH_ENABLED_DEFAULT;

// ---------- Reset flag ----------
volatile bool dsp_needs_reset = false;

size_t DSPStream::readBytes(uint8_t *buffer, size_t length)
{
  size_t n = _source.readBytes(buffer, length);
  if (n == 0) {
    memset(buffer, 0, length);
    return length;
  }
  return n;
}
