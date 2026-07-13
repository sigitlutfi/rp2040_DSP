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

// ---------- EQ Defaults ----------
const float EQ_BASS_FREQ = 200.0f;
const float EQ_BASS_DB = 0.0f;
const float EQ_MID_FREQ = 1000.0f;
const float EQ_MID_DB = 0.0f;
const float EQ_TREBLE_FREQ = 8000.0f;
const float EQ_TREBLE_DB = 0.0f;
