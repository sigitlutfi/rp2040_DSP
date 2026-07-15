#pragma once
#include "AudioTools.h"
#include "config.h"
#include "eq.h"
#include "stereo_width.h"
#include "dsp_limiter.h"

extern volatile bool eq_needs_reinit;
extern volatile bool dsp_needs_reset;
extern EQ3Band eq_state;
extern LimiterState limiter_state;
// extern NoiseGate gate_state;
// extern CompressorState comp_state;
// extern LoudnessState loudness_state;
// extern ReverbState reverb_state;
extern volatile int16_t stereo_width_q8;
extern volatile int16_t usb_volume_q8;

// per-module enable flags
extern bool eq_enabled;
extern bool width_enabled;

class DSPStream : public Stream
{
  Stream &_source;

public:
  DSPStream(Stream &source) : _source(source) {}

  int available() override { return _source.available(); }
  int read() override { return _source.read(); }
  int peek() override { return _source.peek(); }

  size_t readBytes(uint8_t *buffer, size_t length);
  size_t write(uint8_t c) override { return _source.write(c); }
  size_t write(const uint8_t *buffer, size_t length) override { return _source.write(buffer, length); }
};
