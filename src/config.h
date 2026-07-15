#pragma once
#include "AudioTools.h"

// ---------- Audio Config ----------
const AudioInfo AUDIO_INFO(44100, 2, 16);

// ---------- Pins ----------
const int PIN_BCK = 26;
const int PIN_WS = 27;
const int PIN_DATA = 28;
const int LED_PIN = 16;
const int NUM_LEDS = 1;

// ---------- Buffer ----------
const int QUEUE_ENTRIES = 2048;
const int QUEUE_BUF_SIZE = 32;
const int PRE_BUFFER_THRESHOLD = 16384;
const int USB_READ_BUF_SIZE = 512;

// ---------- Streaming timeout ----------
const unsigned long STREAM_TIMEOUT_MS = 2000;

// ---------- Global State (defined in main.cpp) ----------
extern volatile uint32_t usb_bytes_in;
extern volatile uint32_t underflow_count;
extern volatile bool streaming_active;
extern volatile bool pre_buffered;
extern volatile uint32_t current_sample_rate;

// ---------- DSP Bypass ----------
const bool DSP_BYPASS = false; // true = skip all processing

// ---------- Per-module enable (debug: one at a time) ----------
const bool EQ_ENABLED_DEFAULT = true;        // 3-band EQ (float biquad)
const bool WIDTH_ENABLED_DEFAULT = false;    // stereo width (Q8 int)
const bool GATE_ENABLED_DEFAULT = false;     // noise gate
const bool COMP_ENABLED_DEFAULT = false;     // compressor
const bool EXCITER_ENABLED_DEFAULT = false;  // exciter
const bool REVERB_ENABLED_DEFAULT = false;   // reverb
const bool LOUDNESS_ENABLED_DEFAULT = false; // loudness

// ---------- EQ Defaults ----------
const float EQ_BASS_FREQ = 200.0f;
const float EQ_BASS_DB = 3.0f;
const float EQ_BASS_Q = 0.5f;
const float EQ_MID_FREQ = 1000.0f;
const float EQ_MID_DB = 1.0f;
const float EQ_MID_Q = 0.8f;
const float EQ_TREBLE_FREQ = 8000.0f;
const float EQ_TREBLE_DB = 4.2f;
const float EQ_TREBLE_Q = 0.7f;

// ---------- Noise Gate Defaults ----------
const int16_t GATE_THRESHOLD_Q8 = 10; // -40dB
const int16_t GATE_ATTACK = 128;      // fast ~1ms
const int16_t GATE_RELEASE = 10;      // ~50ms

// ---------- Stereo Width Defaults ----------
const int16_t STEREO_WIDTH_Q8 = 628; // 1.5x

// ---------- Loudness Defaults ----------
const bool LOUDNESS_AUTO_DEFAULT = true;    // auto mode on/off
const bool LOUDNESS_ACTIVE_DEFAULT = false; // manual on/off

// ---------- Exciter Defaults ----------
const float EXCITER_MIX = 0.8f; // 0.0=bypass, 1.0=full

// ---------- Compressor Defaults ----------
const int16_t COMPRESSOR_THRESHOLD_Q8 = 82; // -20dB
const int16_t COMPRESSOR_RATIO_Q8 = 1024;   // 4:1
const int16_t COMPRESSOR_ATTACK = 20;       // ~10ms
const int16_t COMPRESSOR_RELEASE = 5;       // ~100ms

// ---------- Gain Defaults ----------
// gain module archived — volume controlled by PC USB slider

// ---------- Limiter Defaults ----------
const int16_t LIMITER_THRESHOLD = 32000; // ~0.98 of max

// ---------- Reverb Defaults ----------
const float REVERB_DELAY_MS = 30.0f; // delay line length (15-60ms)
const float REVERB_FEEDBACK = 0.30f; // feedback amount (10-60%), higher = longer tail
const float REVERB_WET = 0.25f;      // wet/dry mix (0-80%), 0.25 = 25% reverb
const float REVERB_DAMPING = 0.5f;   // high-freq damping (0.0=bright, 1.0=dark), lower = brighter reverb
