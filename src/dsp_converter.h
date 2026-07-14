#pragma once
#include "AudioTools.h"
#include "dsp_stream.h"

class DSPConverter : public BaseConverter
{
public:
  size_t convert(uint8_t *src, size_t byte_count) override;
};
