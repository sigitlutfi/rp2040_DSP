#include "dsp_reverb.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

void reverb_init(ReverbState &r, uint32_t sample_rate, float delay_ms)
{
  r.buf_len = (uint32_t)(sample_rate * delay_ms / 1000.0f);
  if (r.buf_len < 64)
    r.buf_len = 64;

  r.buffer_l = (int16_t *)malloc(r.buf_len * sizeof(int16_t));
  r.buffer_r = (int16_t *)malloc(r.buf_len * sizeof(int16_t));

  memset(r.buffer_l, 0, r.buf_len * sizeof(int16_t));
  memset(r.buffer_r, 0, r.buf_len * sizeof(int16_t));

  r.write_pos = 0;
  r.feedback = REVERB_FEEDBACK;
  r.wet = REVERB_WET;
  r.damping = REVERB_DAMPING;
  r.damp_state_l = 0.0f;
  r.damp_state_r = 0.0f;
  r.active = true;
}

void reverb_reset(ReverbState &r)
{
  if (r.buffer_l)
    memset(r.buffer_l, 0, r.buf_len * sizeof(int16_t));
  if (r.buffer_r)
    memset(r.buffer_r, 0, r.buf_len * sizeof(int16_t));
  r.write_pos = 0;
  r.damp_state_l = 0.0f;
  r.damp_state_r = 0.0f;
}

void reverb_set_wet(ReverbState &r, float wet)
{
  if (wet < 0.0f)
    wet = 0.0f;
  if (wet > 0.80f)
    wet = 0.80f;
  r.wet = wet;
}

void reverb_set_feedback(ReverbState &r, float fb)
{
  if (fb < 0.10f)
    fb = 0.10f;
  if (fb > 0.60f)
    fb = 0.60f;
  r.feedback = fb;
}

void reverb_set_delay(ReverbState &r, uint32_t sample_rate, float ms)
{
  uint32_t new_len = (uint32_t)(sample_rate * ms / 1000.0f);
  if (new_len < 64)
    new_len = 64;
  if (new_len == r.buf_len)
    return;

  free(r.buffer_l);
  free(r.buffer_r);

  r.buf_len = new_len;
  r.buffer_l = (int16_t *)malloc(r.buf_len * sizeof(int16_t));
  r.buffer_r = (int16_t *)malloc(r.buf_len * sizeof(int16_t));

  memset(r.buffer_l, 0, r.buf_len * sizeof(int16_t));
  memset(r.buffer_r, 0, r.buf_len * sizeof(int16_t));
  r.write_pos = 0;
}

void reverb_process(ReverbState &r, int16_t *s16, size_t count)
{
  if (!r.active || r.wet < 0.001f)
    return;

  size_t frames = count / 2;
  float fb = r.feedback;
  float wet = r.wet;
  float dry = 1.0f - wet;
  float damp = r.damping;
  float d_l = r.damp_state_l;
  float d_r = r.damp_state_r;
  uint32_t pos = r.write_pos;
  uint32_t len = r.buf_len;
  int16_t *buf_l = r.buffer_l;
  int16_t *buf_r = r.buffer_r;

  for (size_t i = 0; i < frames; i++)
  {
    int32_t in_l = s16[i * 2];
    int32_t in_r = s16[i * 2 + 1];

    // read delayed sample
    int32_t del_l = buf_l[pos];
    int32_t del_r = buf_r[pos];

    // damping: simple IIR low-pass on delayed signal
    d_l = d_l + damp * ((float)del_l - d_l);
    d_r = d_r + damp * ((float)del_r - d_r);

    // write new sample = input + delayed * feedback
    int32_t new_l = in_l + (int32_t)(d_l * fb);
    int32_t new_r = in_r + (int32_t)(d_r * fb);

    // clip before write
    if (new_l > 32767)
      new_l = 32767;
    else if (new_l < -32768)
      new_l = -32768;
    if (new_r > 32767)
      new_r = 32767;
    else if (new_r < -32768)
      new_r = -32768;

    buf_l[pos] = (int16_t)new_l;
    buf_r[pos] = (int16_t)new_r;

    // output = dry + wet * delayed
    int32_t out_l = (int32_t)(in_l * dry + d_l * wet);
    int32_t out_r = (int32_t)(in_r * dry + d_r * wet);

    if (out_l > 32767)
      out_l = 32767;
    else if (out_l < -32768)
      out_l = -32768;
    if (out_r > 32767)
      out_r = 32767;
    else if (out_r < -32768)
      out_r = -32768;

    s16[i * 2] = (int16_t)out_l;
    s16[i * 2 + 1] = (int16_t)out_r;

    pos++;
    if (pos >= len)
      pos = 0;
  }

  r.write_pos = pos;
  r.damp_state_l = d_l;
  r.damp_state_r = d_r;
}
