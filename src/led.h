#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"

void led_init(Adafruit_NeoPixel &strip);
void led_update(Adafruit_NeoPixel &strip, uint32_t underflow_count);
