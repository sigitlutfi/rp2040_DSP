#include "vu.h"

volatile int16_t vu_peak_l = 0;
volatile int16_t vu_peak_r = 0;

void vu_init()
{
  vu_peak_l = 0;
  vu_peak_r = 0;
}
