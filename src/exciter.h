#pragma once
#include <stdint.h>
#include <stddef.h>

extern bool exciter_enabled;

void exciter_process(int16_t *s16, size_t count);
