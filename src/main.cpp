#include "AudioTools.h"
#include "AudioTools/Communication/USB/USBAudioStream.h"
#include "AudioTools/Concurrency/RP2040.h"
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "dsp_stream.h"
#include "dsp_converter.h"
#include "eq.h"
#include "led.h"
#include "serial_cmd.h"

extern volatile bool eq_pending_update;
extern volatile float eq_pending_db[3];

// ---------- Global State ----------
volatile uint32_t usb_bytes_in = 0;
volatile uint32_t underflow_count = 0;
volatile bool streaming_active = false;
volatile bool pre_buffered = false;
volatile uint32_t current_sample_rate = 44100;
volatile int16_t usb_volume_q8 = 256;

// ---------- Audio Objects ----------
USBAudioStream usb_in;
I2SStream i2s;
BufferRP2040 buffer(QUEUE_ENTRIES, QUEUE_BUF_SIZE);
QueueStream queue(buffer);
DSPStream dsp_stream(queue);
StreamCopy copier(i2s, dsp_stream);
DSPConverter dsp_converter;
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
uint8_t usb_read_buf[USB_READ_BUF_SIZE];
uint8_t usb_tx_buf[USB_READ_BUF_SIZE];

void setup()
{
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000)
    ;

  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);
  Serial.println("=== RP2040 Minimal Soundcard ===");

  led_init(strip);

  limiter_init(limiter_state, LIMITER_THRESHOLD);

  if (!TinyUSBDevice.isInitialized())
    TinyUSBDevice.begin(0);

  queue.begin();
  Serial.println("[core0] Queue buffer ready");

  auto usb_cfg = usb_in.defaultConfig(RXTX_MODE);
  usb_cfg.copyFrom(AUDIO_INFO);
  usb_cfg.volume_active = false;
  usb_cfg.enable_multi_sample_rate = false;
  usb_cfg.vid = 0xCafe;
  usb_cfg.pid = 0x4010;
  usb_cfg.manufacturer = "RP2040";
  usb_cfg.product = "RYNLABS";

  if (!usb_in.begin(usb_cfg))
    Serial.println("[core0] ERROR: USB Audio init gagal");
  else
    Serial.println("[core0] USB Audio ready");

  usb_in.setSampleRateCallback([](uint32_t rate)
                               {
    current_sample_rate = rate;
    eq_needs_reinit = true;
    queue.clear();
    pre_buffered = false;
    AudioInfo newInfo(rate, 2, 16);
    i2s.setAudioInfo(newInfo); });

  usb_in.setStreamingStateCallback([](bool tx, bool rx)
                                   {
    streaming_active = rx;
    if (!rx)
    {
      queue.clear();
      pre_buffered = false;
    } });

  auto i2s_cfg = i2s.defaultConfig(TX_MODE);
  i2s_cfg.copyFrom(AUDIO_INFO);
  i2s_cfg.pin_bck = PIN_BCK;
  i2s_cfg.pin_ws = PIN_WS;
  i2s_cfg.pin_data = PIN_DATA;
  if (!i2s.begin(i2s_cfg))
    Serial.printf("[core0] ERROR: I2S init gagal\n");
  else
    Serial.printf("[core0] I2S ready (BCK=%d WS=%d DATA=%d)\n",
                  PIN_BCK, PIN_WS, PIN_DATA);

  if (TinyUSBDevice.mounted())
  {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
    Serial.println("[core0] USB re-enumerated");
  }

  serial_cmd_init();
  Serial.println("[core0] Setup selesai.");
}

void process_pending_eq()
{
  if (!eq_pending_update)
    return;

  float freqs[] = {EQ_BASS_FREQ, EQ_MID_FREQ, EQ_TREBLE_FREQ};
  float qs[] = {EQ_BASS_Q, EQ_MID_Q, EQ_TREBLE_Q};
  for (int b = 0; b < 3; b++)
    eq_set_band(eq_state, b, freqs[b], eq_pending_db[b], qs[b], current_sample_rate);

  eq_pending_update = false;
}

void loop()
{
  size_t n = usb_in.readBytes(usb_read_buf, sizeof(usb_read_buf));
  if (n > 0)
  {
    queue.write(usb_read_buf, n);
    usb_bytes_in += n;
  }

  // Pre-buffer: wait for enough data before starting playback
  if (streaming_active && !pre_buffered)
  {
    if (buffer.available() >= PRE_BUFFER_THRESHOLD)
    {
      pre_buffered = true;
      dsp_needs_reset = true; // reset DSP states to avoid transient
    }
  }

  process_pending_eq();

  // Poll USB host volume → integer Q8 for DSP chain
  float vol = usb_in.volume(1);
  bool muted = usb_in.isMute(1);
  usb_volume_q8 = muted ? 0 : (int16_t)(vol * 256.0f);

  led_update(strip, underflow_count);
  serial_cmd_process();

  memset(usb_tx_buf, 0, sizeof(usb_tx_buf));
  usb_in.write(usb_tx_buf, sizeof(usb_tx_buf));
}

void setup1() {}

void loop1()
{
  if (!pre_buffered)
    return;
  copier.copy(dsp_converter);
}
