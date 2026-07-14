#include "dsp_stream.h"

// ---------- EQ ----------
volatile bool eq_needs_reinit = false;
EQ3Band eq_state;

// ---------- Stereo Width ----------
extern volatile int16_t stereo_width_q8;

// ---------- Compressor ----------
CompressorState comp_state;

// ---------- Gain ----------
GainState gain_state;

// ---------- Limiter ----------
LimiterState limiter_state;

// ---------- Noise Gate ----------
NoiseGate gate_state;

// ---------- Loudness ----------
LoudnessState loudness_state;

// ---------- Reverb ----------
ReverbState reverb_state;

// ---------- Reset flag ----------
volatile bool dsp_needs_reset = false;

size_t DSPStream::readBytes(uint8_t *buffer, size_t length)
{
  size_t n = _source.readBytes(buffer, length);
  if (n > 0)
  {
    if (eq_needs_reinit)
    {
      eq_init(eq_state, current_sample_rate);
      eq_needs_reinit = false;
    }

    // Reset all DSP states when playback starts (avoid transient from zero history)
    if (dsp_needs_reset)
    {
      // Re-init all states using config.h defaults
      compressor_init(comp_state, COMPRESSOR_THRESHOLD_Q8, COMPRESSOR_RATIO_Q8);
      gain_init(gain_state, GAIN_Q8);
      limiter_init(limiter_state, LIMITER_THRESHOLD);
      noise_gate_init(gate_state, GATE_THRESHOLD_Q8);
      loudness_init(loudness_state, current_sample_rate);
      stereo_width_q8 = STEREO_WIDTH_Q8;
      reverb_reset(reverb_state);
      eq_init(eq_state, current_sample_rate);

      // Clear biquad history
      for (int ch = 0; ch < 2; ch++)
      {
        for (int b = 0; b < 3; b++)
        {
          eq_state.band[ch][b].x1 = eq_state.band[ch][b].x2 = 0;
          eq_state.band[ch][b].y1 = eq_state.band[ch][b].y2 = 0;
        }
        loudness_state.bass_shelf[ch].x1 = loudness_state.bass_shelf[ch].x2 = 0;
        loudness_state.bass_shelf[ch].y1 = loudness_state.bass_shelf[ch].y2 = 0;
        loudness_state.treble_shelf[ch].x1 = loudness_state.treble_shelf[ch].x2 = 0;
        loudness_state.treble_shelf[ch].y1 = loudness_state.treble_shelf[ch].y2 = 0;
      }

      dsp_needs_reset = false;
    }

    if (!DSP_BYPASS)
    {
      // Noise Gate (first - cut noise before processing)
      noise_gate_process(gate_state, (int16_t *)buffer, n / 2);

      eq_process(eq_state, (int16_t *)buffer, n / 2);

      stereo_width_process((int16_t *)buffer, n / 2, stereo_width_q8);

      // Loudness: auto boost at low PC volume
      loudness_process(loudness_state, (int16_t *)buffer, n / 2);

      exciter_process((int16_t *)buffer, n / 2);

      // Reverb (after exciter, before compressor)
      reverb_process(reverb_state, (int16_t *)buffer, n / 2);

      // Compressor (before gain)
      compressor_process(comp_state, (int16_t *)buffer, n / 2);

      // Gain
      gain_process(gain_state, (int16_t *)buffer, n / 2);

      // Limiter
      limiter_process(limiter_state, (int16_t *)buffer, n / 2);

      // Dither
      dither_process((int16_t *)buffer, n / 2);
    }
  }
  return n;
}
