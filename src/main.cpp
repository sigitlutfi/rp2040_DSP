#include "AudioTools.h"
#include "AudioTools/Communication/USB/USBAudioStream.h"
#include "AudioTools/Concurrency/RP2040.h"
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "dsp_stream.h"
#include "dsp_reverb.h"
#include "led.h"
#include "serial_cmd.h"

// Nanti kalau PCM1808 ada, tinggal ganti memset(usb_tx_buf, 0, ...) → baca dari I2S RX PCM1808.

// ---------- Global State ----------
volatile uint32_t usb_bytes_in = 0;
volatile uint32_t underflow_count = 0;
volatile bool streaming_active = false;
volatile bool pre_buffered = false;
volatile uint32_t current_sample_rate = 44100;

// ---------- Audio Objects ----------
USBAudioStream usb_in;
I2SStream i2s;
BufferRP2040 buffer(QUEUE_ENTRIES, QUEUE_BUF_SIZE);
QueueStream queue(buffer);
DSPStream dsp_stream(queue);
StreamCopy copier(i2s, dsp_stream);
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
  Serial.println("=== RP2040 USB Soundcard + Gain ===");

  led_init(strip);

  if (!TinyUSBDevice.isInitialized())
    TinyUSBDevice.begin(0);

  queue.begin();
  Serial.println("[core0] Queue buffer ready (32KB)");

  // Init reverb (heap allocation, uses config.h defaults)
  reverb_init(reverb_state, 44100, REVERB_DELAY_MS);
  Serial.printf("[core0] Reverb ready (delay=%dms wet=%.0f%% fb=%.0f%%)\n",
                (int)REVERB_DELAY_MS, REVERB_WET * 100, REVERB_FEEDBACK * 100);

  auto usb_cfg = usb_in.defaultConfig(RXTX_MODE);
  usb_cfg.copyFrom(AUDIO_INFO);
  usb_cfg.enable_interrupt_ep = true;
  usb_cfg.enable_multi_sample_rate = false;
  usb_cfg.vid = 0xCafe;
  usb_cfg.pid = 0x4010;
  usb_cfg.manufacturer = "RP2040";
  usb_cfg.product = "RYNLABS";

  if (!usb_in.begin(usb_cfg))
    Serial.println("[core0] ERROR: USB Audio init gagal");
  else
    Serial.println("[core0] USB Audio (RXTX) ready [44.1kHz, interrupt EP]");

  usb_in.setSampleRateCallback([](uint32_t rate)
                               {
    current_sample_rate = rate;
    eq_needs_reinit = true;
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

  // USB volume callback: auto-toggle loudness when PC volume < 40%
  usb_in.setVolumeCallback([](float vol, uint8_t ch)
                           {
    if (ch == 0) // master channel only
      loudness_set_usb_volume(loudness_state, vol, current_sample_rate);
  });

  auto i2s_cfg = i2s.defaultConfig(TX_MODE);
  i2s_cfg.copyFrom(AUDIO_INFO);
  i2s_cfg.pin_bck = PIN_BCK;
  i2s_cfg.pin_ws = PIN_WS;
  i2s_cfg.pin_data = PIN_DATA;
  if (!i2s.begin(i2s_cfg))
    Serial.println("[core0] ERROR: I2S init gagal");
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
  Serial.println("[core0] DSPStream: Gate → EQ → Width → Loudness → Exciter → Reverb → Comp → Gain → Limit → Dither (Core1)");
  Serial.println("[core0] USB TX: Mic input (silence placeholder)");
  Serial.println("[core0] Loudness: auto on < 40% vol | Cmd: loud on/off/auto/<0-100>");
  Serial.println("[core0] Reverb: ON by default | Cmd: rvb on/off/w/f/d");
  Serial.println("[core0] Cmd: g/l/w<1.0-3.0> | eb/em/et<-12~12dB> | status");
  Serial.println("[core0] Setup selesai.");
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
  copier.copy();
}
