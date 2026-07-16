#include "oled.h"
#include "config.h"
#include "dsp_stream.h"
#include "dsp_converter.h"
#include "vu.h"
#include "eq.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

volatile bool oled_dirty = true;
volatile uint8_t oled_mode = 0; // 0=dashboard, 1=VU

// VU peak hold + decay
static int16_t vu_hold_l = 0;
static int16_t vu_hold_r = 0;
static uint8_t vu_hold_timer_l = 0;
static uint8_t vu_hold_timer_r = 0;

void oled_init()
{
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    Serial.println("SSD1306 init failed");
    return;
  }
  display.clearDisplay();
  display.display();
  Serial.println("SSD1306 ready (400kHz)");
}

void oled_splash()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(16, 8);
  display.println("RYNLABS");

  display.setTextSize(1);
  display.setCursor(28, 32);
  display.println("USB Audio DSP");

  display.setCursor(48, 48);
  display.println("v1.0.0");

  display.display();
}

// ---- Dashboard ----
static void draw_dashboard()
{
  display.clearDisplay();
  display.setTextSize(1);

  // Row 0: bitrate
  display.setCursor(0, 0);
  display.printf("%luHz stereo 16bit", current_sample_rate);

  // Row 1: power + limiter
  display.setCursor(0, 10);
  display.printf("PWR:%d%% LIM:%d",
                 (int)(usb_volume_q8 * 100 / 256),
                 limiter_state.threshold);

  // display.drawFastHLine(0, 20, SCREEN_WIDTH, SSD1306_WHITE);

  // Rows 2-4: EQ bars
  struct EqBar
  {
    const char *label;
    float db;
  };
  EqBar bars[] = {
      {"LSLF", eq_state.db[0]},
      {"PEAK", eq_state.db[1]},
      {"HSLF", eq_state.db[2]},
  };

  for (int i = 0; i < 3; i++)
  {
    int y = 20 + i * 12;

    // label
    display.setCursor(0, y);
    display.print(bars[i].label);

    // bar: x=28..127 = 99px, center = x=77
    int bar_x = 28, bar_w = 99;
    int cx = bar_x + bar_w / 2; // center x = 77
    display.drawRect(bar_x, y, bar_w, 9, SSD1306_WHITE);

    // center marker
    display.drawFastVLine(cx, y + 1, 7, SSD1306_WHITE);

    // bidirectional fill
    int fill = (int)(bars[i].db * (bar_w / 2) / 12.0f);
    if (fill > 0)
      display.fillRect(cx + 1, y + 1, fill, 7, SSD1306_WHITE);
    else if (fill < 0)
      display.fillRect(cx + fill, y + 1, -fill, 7, SSD1306_WHITE);

    // dB text
    display.setCursor(102, y);
    display.printf("%+.0f", bars[i].db);
  }

  // display.drawFastHLine(0, 58, SCREEN_WIDTH, SSD1306_WHITE);

  // Row 5: signal status
  display.setCursor(0, 54);
  if (!streaming_active)
    display.print("STOP");
  else if (!pre_buffered)
    display.print("BUFFERING..");
  else
    display.print("BURNING !!!!!");
}

// ---- VU Meter (horizontal) ----
static void draw_vu()
{
  display.clearDisplay();
  display.setTextSize(1);

  // Labels
  display.setCursor(0, 0);
  display.print("VU");

  display.setCursor(110, 0);
  display.printf("%lu", current_sample_rate);

  // --- L channel ---
  display.setCursor(0, 14);
  display.print("L");

  // Bar area: x=10..127 (117px), y=14..26 (12px high)
  int bar_x = 10;
  int bar_w = 117;
  int bar_y = 14;
  int bar_h = 12;

  // Border
  display.drawRect(bar_x, bar_y, bar_w, bar_h, SSD1306_WHITE);

  // Peak level → bar width (0..115 inner)
  int level_l = (int)vu_peak_l * 115 / 32767;
  if (level_l > 115)
    level_l = 115;
  if (level_l > 0)
    display.fillRect(bar_x + 1, bar_y + 1, level_l, bar_h - 2, SSD1306_WHITE);

  // Peak hold
  if (vu_peak_l > vu_hold_l)
  {
    vu_hold_l = vu_peak_l;
    vu_hold_timer_l = 15;
  }
  else if (vu_hold_timer_l > 0)
  {
    vu_hold_timer_l--;
  }
  else
  {
    if (vu_hold_l > 0)
      vu_hold_l -= 200;
  }
  int hold_l = vu_hold_l * 115 / 32767;
  if (hold_l > 115)
    hold_l = 115;
  if (hold_l > 0)
    display.drawFastVLine(bar_x + 1 + hold_l, bar_y + 1, bar_h - 2, SSD1306_WHITE);

  // --- R channel ---
  display.setCursor(0, 32);
  display.print("R");

  int bar_y2 = 32;

  display.drawRect(bar_x, bar_y2, bar_w, bar_h, SSD1306_WHITE);

  int level_r = (int)vu_peak_r * 115 / 32767;
  if (level_r > 115)
    level_r = 115;
  if (level_r > 0)
    display.fillRect(bar_x + 1, bar_y2 + 1, level_r, bar_h - 2, SSD1306_WHITE);

  if (vu_peak_r > vu_hold_r)
  {
    vu_hold_r = vu_peak_r;
    vu_hold_timer_r = 15;
  }
  else if (vu_hold_timer_r > 0)
  {
    vu_hold_timer_r--;
  }
  else
  {
    if (vu_hold_r > 0)
      vu_hold_r -= 200;
  }
  int hold_r = vu_hold_r * 115 / 32767;
  if (hold_r > 115)
    hold_r = 115;
  if (hold_r > 0)
    display.drawFastVLine(bar_x + 1 + hold_r, bar_y2 + 1, bar_h - 2, SSD1306_WHITE);

  // dB text
  display.setCursor(0, 50);
  float db_l = vu_peak_l > 0 ? 20.0f * log10f((float)vu_peak_l / 32767.0f) : -96.0f;
  float db_r = vu_peak_r > 0 ? 20.0f * log10f((float)vu_peak_r / 32767.0f) : -96.0f;
  display.printf("L:%.0fdB R:%.0fdB", db_l, db_r);
}

// ---- Public API ----
void oled_update()
{
  if (!oled_dirty && oled_mode != 1)
    return;

  if (oled_mode == 1)
  {
    static unsigned long last_vu = 0;
    if (millis() - last_vu < 100)
      return; // 3 FPS
    last_vu = millis();
  }

  oled_dirty = false;

  if (oled_mode == 0)
    draw_dashboard();
  else
    draw_vu();

  display.display();
}
