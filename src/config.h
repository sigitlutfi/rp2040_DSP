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

// ---------- Global State (defined in main.cpp) ----------
extern volatile uint32_t usb_bytes_in;
extern volatile uint32_t underflow_count;
extern volatile bool streaming_active;
extern volatile bool pre_buffered;
extern volatile uint32_t current_sample_rate;

// ---------- DSP Bypass ----------
const bool DSP_BYPASS = false;

// ---------- Per-module enable ----------
const bool EQ_ENABLED_DEFAULT = true;
const bool WIDTH_ENABLED_DEFAULT = false;

// ---------- EQ Defaults (3-band) ----------
const float EQ_BASS_FREQ = 200.0f;
const float EQ_BASS_DB = 3.0f;
const float EQ_BASS_Q = 0.5f;
const float EQ_MID_FREQ = 1000.0f;
const float EQ_MID_DB = 1.0f;
const float EQ_MID_Q = 0.8f;
const float EQ_TREBLE_FREQ = 8000.0f;
const float EQ_TREBLE_DB = 4.2f;
const float EQ_TREBLE_Q = 0.7f;

// ---------- Stereo Width Defaults ----------
const int16_t STEREO_WIDTH_Q8 = 307; // 307/256 = 1.1x

// ---------- Limiter Defaults ----------
const int16_t LIMITER_THRESHOLD = 32000;
