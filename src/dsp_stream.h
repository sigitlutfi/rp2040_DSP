#pragma once
#include "AudioTools.h"
#include "config.h"
#include "eq.h"
#include "stereo_width.h"
#include "exciter.h"
#include "dither.h"

extern volatile bool eq_needs_reinit;
extern EQ3Band eq_state;

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
