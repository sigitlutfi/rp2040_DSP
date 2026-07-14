#include "serial_cmd.h"
#include "AudioTools.h"
#include "config.h"
#include "eq.h"
#include "dsp_stream.h"

extern GainState gain_state;
extern LimiterState limiter_state;
extern LoudnessState loudness_state;
extern ReverbState reverb_state;
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
          gain_state.gain_q8 = (int16_t)(val * 256);
          Serial.printf("[gain] = %.2fx\n", val);
        }
      }
      else if (line.startsWith("l "))
      {
        int val = line.substring(2).toInt();
        if (val >= 1000 && val <= 32767)
        {
          limiter_state.threshold = val;
          Serial.printf("[limit] = %d (%.0f%%)\n", limiter_state.threshold,
                        limiter_state.threshold * 100.0 / 32767);
        }
      }
      else if (line.startsWith("eb "))
      {
        float val = line.substring(3).toFloat();
        if (val >= -12.0f && val <= 12.0f)
        {
          eq_set_band(eq_state, 0, EQ_BASS_FREQ, val, EQ_BASS_Q, current_sample_rate);
          Serial.printf("[eq bass] = %.1f dB @ %.0fHz\n", val, EQ_BASS_FREQ);
        }
      }
      else if (line.startsWith("em "))
      {
        float val = line.substring(3).toFloat();
        if (val >= -12.0f && val <= 12.0f)
        {
          eq_set_band(eq_state, 1, EQ_MID_FREQ, val, EQ_MID_Q, current_sample_rate);
          Serial.printf("[eq mid] = %.1f dB @ %.0fHz\n", val, EQ_MID_FREQ);
        }
      }
      else if (line.startsWith("et "))
      {
        float val = line.substring(3).toFloat();
        if (val >= -12.0f && val <= 12.0f)
        {
          eq_set_band(eq_state, 2, EQ_TREBLE_FREQ, val, EQ_TREBLE_Q, current_sample_rate);
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
      else if (line == "loud auto")
      {
        loudness_set_manual(loudness_state, false, 0, current_sample_rate);
        loudness_state.auto_mode = true;
        Serial.printf("[loudness] auto mode (activates < 40%% vol)\n");
      }
      else if (line == "loud on")
      {
        loudness_set_manual(loudness_state, true, 50, current_sample_rate);
        loudness_state.auto_mode = false;
        Serial.printf("[loudness] manual ON (50%%)\n");
      }
      else if (line == "loud off")
      {
        loudness_set_manual(loudness_state, false, 0, current_sample_rate);
        loudness_state.auto_mode = false;
        Serial.printf("[loudness] OFF\n");
      }
      else if (line.startsWith("loud "))
      {
        int val = line.substring(5).toInt();
        if (val >= 0 && val <= 100)
        {
          loudness_set_manual(loudness_state, val > 0, (uint8_t)val, current_sample_rate);
          loudness_state.auto_mode = false;
          Serial.printf("[loudness] manual %s (%d%%)\n", val > 0 ? "ON" : "OFF", val);
        }
      }
      else if (line == "eq")
      {
        Serial.printf("[eq] bass=%.1f dB mid=%.1f dB treble=%.1f dB\n",
                      EQ_BASS_DB, EQ_MID_DB, EQ_TREBLE_DB);
      }
      else if (line == "rvb on")
      {
        reverb_state.active = true;
        Serial.printf("[reverb] ON (wet=%.0f%% fb=%.0f%% delay=%dms)\n",
                      reverb_state.wet * 100, reverb_state.feedback * 100,
                      (int)(reverb_state.buf_len * 1000.0f / current_sample_rate));
      }
      else if (line == "rvb off")
      {
        reverb_state.active = false;
        Serial.printf("[reverb] OFF\n");
      }
      else if (line.startsWith("rvb w"))
      {
        float val = line.substring(5).toFloat();
        if (val >= 0.0f && val <= 80.0f)
        {
          reverb_set_wet(reverb_state, val / 100.0f);
          Serial.printf("[reverb] wet = %.0f%%\n", val);
        }
      }
      else if (line.startsWith("rvb f"))
      {
        float val = line.substring(5).toFloat();
        if (val >= 10.0f && val <= 60.0f)
        {
          reverb_set_feedback(reverb_state, val / 100.0f);
          Serial.printf("[reverb] feedback = %.0f%%\n", val);
        }
      }
      else if (line.startsWith("rvb d"))
      {
        int val = line.substring(5).toInt();
        if (val >= 15 && val <= 60)
        {
          reverb_set_delay(reverb_state, current_sample_rate, (float)val);
          Serial.printf("[reverb] delay = %dms\n", val);
        }
      }
      else if (line == "status")
      {
        Serial.printf("gain=%.2fx limit=%d (%.0f%%)\n",
                      gain_state.gain_q8 / 256.0, limiter_state.threshold,
                      limiter_state.threshold * 100.0 / 32767);
        Serial.printf("loudness=%s %d%% usb_vol=%.0f%% underflow=%lu rate=%luHz\n",
                      loudness_state.active ? "ON" : "off",
                      loudness_state.intensity,
                      loudness_state.usb_volume * 100.0f,
                      underflow_count, current_sample_rate);
        Serial.printf("reverb=%s wet=%.0f%% fb=%.0f%%\n",
                      reverb_state.active ? "ON" : "off",
                      reverb_state.wet * 100, reverb_state.feedback * 100);
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
