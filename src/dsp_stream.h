#pragma once
#include "AudioTools.h"
#include "config.h"
#include "eq.h"
#include "stereo_width.h"
#include "exciter.h"
#include "compressor.h"
#include "dsp_gain.h"
#include "dsp_limiter.h"
#include "noise_gate.h"
#include "loudness.h"
#include "dsp_reverb.h"
#include "dither.h"

extern volatile bool eq_needs_reinit;
extern volatile bool dsp_needs_reset;
extern EQ3Band eq_state;
extern GainState gain_state;
extern LimiterState limiter_state;
extern LoudnessState loudness_state;
extern ReverbState reverb_state;

class DSPStream : public Stream
{
  Stream &_source;

public:
  DSPStream(Stream &source) : _source(source) {}

  int available() { return _source.available(); }
  int read() { return _source.read(); }
  int peek() { return _source.peek(); }

  size_t readBytes(uint8_t *buffer, size_t length);
  size_t write(uint8_t c) { return _source.write(c); }
  size_t write(const uint8_t *buffer, size_t length) { return _source.write(buffer, length); }
};
