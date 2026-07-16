#pragma once
#include <stdint.h>

extern volatile bool oled_dirty;
extern volatile uint8_t oled_mode;

void oled_init();
void oled_splash();
void oled_update();
