#include "AudioTools.h"
#include "AudioTools/Communication/USB/USBAudioStream.h"
#include "AudioTools/Concurrency/RP2040.h"
#include <Adafruit_NeoPixel.h>

// ---------- Audio Config ----------
AudioInfo info(44100, 2, 16);

// ---------- Pins ----------
const int PIN_BCK = 26;
const int PIN_WS = 27;
const int PIN_DATA = 28;
const int LED_PIN = 16;
const int NUM_LEDS = 1;
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Pre-buffer: ~16KB threshold (~91ms @ 44.1kHz)
const int PRE_BUFFER_THRESHOLD = 16384;

// ---------- Integer-only Effects (Q8 fixed-point) ----------
volatile int16_t gain_q8 = 307; // 1.2x
volatile int16_t limit_threshold = 32000;

// ---------- GainStream: applies gain during copier read ----------
class GainStream : public Stream
{
  Stream &_source;

public:
  GainStream(Stream &source) : _source(source) {}

  int available() { return _source.available(); }
  int read() { return _source.read(); }
  int peek() { return _source.peek(); }

  size_t readBytes(uint8_t *buffer, size_t length)
  {
    size_t n = _source.readBytes(buffer, length);
    if (n > 0)
    {
      int16_t *s16 = (int16_t *)buffer;
      size_t count = n / 2;
      int16_t g = gain_q8;
      int16_t lt = limit_threshold;
      for (size_t i = 0; i < count; i++)
      {
        int32_t s = ((int32_t)s16[i] * g) >> 8;
        if (s > lt) s = lt;
        else if (s < -lt) s = -lt;
        s16[i] = (int16_t)s;
      }
    }
    return n;
  }

  size_t write(uint8_t c) { return _source.write(c); }
  size_t write(const uint8_t *buffer, size_t length) { return _source.write(buffer, length); }
};

// ---------- Audio Objects ----------
USBAudioStream usb_in;
I2SStream i2s;

BufferRP2040 buffer(1024, 32);
QueueStream queue(buffer);

GainStream gain_stream(queue);
StreamCopy copier(i2s, gain_stream);

// USB read buffer
uint8_t usb_read_buf[512];

// ---------- State ----------
volatile uint32_t usb_bytes_in = 0;
volatile uint32_t i2s_bytes_out = 0;
volatile uint32_t underflow_count = 0;
volatile bool streaming_active = false;
volatile bool pre_buffered = false;
volatile uint32_t current_sample_rate = 44100;
unsigned long last_log_time = 0;
uint32_t last_underflow_count = 0;
unsigned long last_underflow_blink = 0;
bool underflow_blink_state = false;
unsigned long last_strip_update = 0;

// ---------- Serial Command Parser ----------
void processSerialCommand()
{
  if (!Serial.available())
    return;

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
  else if (line == "status")
  {
    Serial.printf("gain=%.2fx limit=%d (%.0f%%) underflow=%lu\n",
                  gain_q8 / 256.0, limit_threshold,
                  limit_threshold * 100.0 / 32767, underflow_count);
  }
}

void setup()
{
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000)
    ;

  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

  Serial.println("=== RP2040 USB Soundcard + Gain ===");

  strip.begin();
  strip.setBrightness(40);
  strip.show();

  if (!TinyUSBDevice.isInitialized())
  {
    TinyUSBDevice.begin(0);
  }

  queue.begin();
  Serial.println("[core0] Queue buffer ready (32KB)");

  // --- USB Audio Config ---
  auto usb_cfg = usb_in.defaultConfig(RX_MODE);
  usb_cfg.copyFrom(info);
  usb_cfg.enable_interrupt_ep = true;
  usb_cfg.enable_multi_sample_rate = false;
  usb_cfg.vid = 0xCafe;
  usb_cfg.pid = 0x4010;
  usb_cfg.manufacturer = "RP2040";
  usb_cfg.product = "KONTOL";

  if (!usb_in.begin(usb_cfg))
  {
    Serial.println("[core0] ERROR: USB Audio init gagal");
  }
  else
  {
    Serial.println("[core0] USB Audio (RX) ready [44.1kHz, interrupt EP]");
  }

  usb_in.setSampleRateCallback([](uint32_t rate)
                               {
    current_sample_rate = rate;
    queue.flush();
    pre_buffered = false;
    AudioInfo newInfo(rate, 2, 16);
    i2s.setAudioInfo(newInfo); });

  usb_in.setStreamingStateCallback([](bool tx, bool rx)
                                   {
    streaming_active = rx;
    if (!rx)
    {
      queue.flush();
      pre_buffered = false;
    } });

  // --- I2S Config ---
  auto i2s_cfg = i2s.defaultConfig(TX_MODE);
  i2s_cfg.copyFrom(info);
  i2s_cfg.pin_bck = PIN_BCK;
  i2s_cfg.pin_ws = PIN_WS;
  i2s_cfg.pin_data = PIN_DATA;
  if (!i2s.begin(i2s_cfg))
  {
    Serial.println("[core0] ERROR: I2S init gagal");
  }
  else
  {
    Serial.printf("[core0] I2S ready (BCK=%d WS=%d DATA=%d)\n",
                  PIN_BCK, PIN_WS, PIN_DATA);
  }

  if (TinyUSBDevice.mounted())
  {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
    Serial.println("[core0] USB re-enumerated");
  }

  Serial.println("[core0] GainStream: 1.0x + limiter (Core1)");
  Serial.println("[core0] Cmd: g<0.1-4.0> | l<1000-32767> | status");
  Serial.println("[core0] Setup selesai.");
}

void loop()
{
  // Core0: USB → queue (murni baseline, zero processing)
  size_t n = usb_in.readBytes(usb_read_buf, sizeof(usb_read_buf));
  if (n > 0)
  {
    queue.write(usb_read_buf, n);
    usb_bytes_in += n;
  }

  // WS2812 RGB: 3 mode
  bool need_update = false;
  if (streaming_active)
  {
    if (underflow_count != last_underflow_count)
    {
      last_underflow_count = underflow_count;
      last_underflow_blink = millis();
      underflow_blink_state = true;
    }
    if (underflow_blink_state && (millis() - last_underflow_blink < 300))
    {
      strip.setPixelColor(0, strip.Color(255, 0, 0));
    }
    else
    {
      underflow_blink_state = false;
      strip.setPixelColor(0, strip.Color(0, 0, 40));
    }
    need_update = true;
  }
  else
  {
    bool on = (millis() / 500) % 2;
    strip.setPixelColor(0, on ? strip.Color(0, 0, 40) : strip.Color(0, 0, 0));
    need_update = true;
  }
  if (need_update && millis() - last_strip_update >= 20)
  {
    last_strip_update = millis();
    strip.show();
  }

  // Serial commands + status log (idle only)
  if (!streaming_active)
  {
    processSerialCommand();
    if (millis() - last_log_time > 2000)
    {
      last_log_time = millis();
      Serial.printf("[idle] underflow=%lu | rate=%luHz\n",
                    underflow_count, current_sample_rate);
    }
  }
}

void setup1()
{
}

void loop1()
{
  // Core1: copier.copy() reads from gain_stream → reads from queue → applies gain → writes to I2S
  copier.copy();
}
