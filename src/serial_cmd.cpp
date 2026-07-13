#include "serial_cmd.h"
#include "AudioTools.h"
#include "config.h"
#include "eq.h"
#include "dsp_stream.h"

extern volatile int16_t gain_q8;
extern volatile int16_t limit_threshold;
extern volatile int16_t stereo_width_q8;

static unsigned long last_log_time = 0;

void serial_cmd_init()
{
  // Nothing to init
}

void serial_cmd_process()
{
  if (!streaming_active)
  {
    if (Serial.available())
    {
      String line = Serial.readStringUntil('\n');
      line.trim();

      if (line.startsWith("g "))
      {
        float val = line.substring(2).toFloat();
        if (val >= 0.1f && val <= 4.0f)
        {
          gain_q8 = (int16_t)(val * 256);
          Serial.printf("[gain] = %.2fx\n", val);
        }
      }
      else if (line.startsWith("l "))
      {
        int val = line.substring(2).toInt();
        if (val >= 1000 && val <= 32767)
        {
          limit_threshold = val;
          Serial.printf("[limit] = %d (%.0f%%)\n", limit_threshold,
                        limit_threshold * 100.0 / 32767);
        }
      }
      else if (line.startsWith("eb "))
      {
        float val = line.substring(3).toFloat();
        if (val >= -12.0f && val <= 12.0f)
        {
          eq_set_band(eq_state, 0, EQ_BASS_FREQ, val, 0.707f, current_sample_rate);
          Serial.printf("[eq bass] = %.1f dB @ %.0fHz\n", val, EQ_BASS_FREQ);
        }
      }
      else if (line.startsWith("em "))
      {
        float val = line.substring(3).toFloat();
        if (val >= -12.0f && val <= 12.0f)
        {
          eq_set_band(eq_state, 1, EQ_MID_FREQ, val, 1.0f, current_sample_rate);
          Serial.printf("[eq mid] = %.1f dB @ %.0fHz\n", val, EQ_MID_FREQ);
        }
      }
      else if (line.startsWith("et "))
      {
        float val = line.substring(3).toFloat();
        if (val >= -12.0f && val <= 12.0f)
        {
          eq_set_band(eq_state, 2, EQ_TREBLE_FREQ, val, 0.707f, current_sample_rate);
          Serial.printf("[eq treble] = %.1f dB @ %.0fHz\n", val, EQ_TREBLE_FREQ);
        }
      }
      else if (line.startsWith("w "))
      {
        float val = line.substring(2).toFloat();
        if (val >= 1.0f && val <= 3.0f)
        {
          stereo_width_q8 = (int16_t)(val * 256);
          Serial.printf("[width] = %.1fx\n", val);
        }
      }
      else if (line == "eq")
      {
        Serial.printf("[eq] bass=%.1f dB mid=%.1f dB treble=%.1f dB\n",
                      EQ_BASS_DB, EQ_MID_DB, EQ_TREBLE_DB);
      }
      else if (line == "status")
      {
        Serial.printf("gain=%.2fx limit=%d (%.0f%%) underflow=%lu rate=%luHz\n",
                      gain_q8 / 256.0, limit_threshold,
                      limit_threshold * 100.0 / 32767, underflow_count,
                      current_sample_rate);
      }
    }

    if (millis() - last_log_time > 2000)
    {
      last_log_time = millis();
      Serial.printf("[idle] underflow=%lu | rate=%luHz\n",
                    underflow_count, current_sample_rate);
    }
  }
}
