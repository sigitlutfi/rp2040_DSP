#pragma once
#include <stdint.h>
#include <stddef.h>

struct ReverbState
{
  int16_t *buffer_l;
  int16_t *buffer_r;
  uint32_t buf_len;
  uint32_t write_pos;
  float feedback;
  float wet;
  float damping;
  float damp_state_l;
  float damp_state_r;
  bool active;
};

void reverb_init(ReverbState &r, uint32_t sample_rate, float delay_ms);
void reverb_process(ReverbState &r, int16_t *s16, size_t count);
void reverb_set_wet(ReverbState &r, float wet);
void reverb_set_feedback(ReverbState &r, float fb);
void reverb_set_delay(ReverbState &r, uint32_t sample_rate, float ms);
void reverb_reset(ReverbState &r);
