#pragma once
#include <stdint.h>
#include <stddef.h>

void stereo_width_process(int16_t *s16, size_t count, int16_t width_q8);
