#pragma once
#include "AudioTools.h"
#include "dsp_stream.h"

extern volatile uint32_t dsp_cpu_us;
extern volatile uint16_t dsp_block_bytes;

class DSPConverter : public BaseConverter
{
public:
  size_t convert(uint8_t *src, size_t byte_count) override;
};
