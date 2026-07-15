#include "oled.h"
#include "config.h"
#include "dsp_stream.h"
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

void oled_init()
{
  Wire.setClock(100000);
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

static void draw_hline(int y)
{
  for (int x = 0; x < SCREEN_WIDTH; x++)
    display.drawPixel(x, y, SSD1306_WHITE);
}

void oled_update()
{
  if (!oled_dirty) return;
  oled_dirty = false;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Line 1: sample rate + format
  display.setCursor(0, 0);
  display.printf("%luHz stereo 16bit", current_sample_rate);

  // Line 2: EQ status
  display.setCursor(0, 14);
  display.printf("EQ:%s L:%.0f M:%.0f H:%.0f",
                 eq_enabled ? "ON" : "OFF",
                 eq_state.db[0], eq_state.db[1], eq_state.db[2]);

  draw_hline(26);

  // Line 3: width + limiter
  display.setCursor(0, 30);
  display.printf("WIDTH: %.1f", stereo_width_q8 / 256.0f);

  // Line 4: volume
  display.setCursor(0, 42);
  display.printf("POWER: %d%%", (int)(usb_volume_q8 * 100 / 256));

  draw_hline(54);

  // Bottom: status
  display.setCursor(0, 56);
  if (!streaming_active)
    display.print("NO SIGNAL");
  else if (!pre_buffered)
    display.print("BUFFERING...");
  else
    display.printf("LIM: %d", limiter_state.threshold);

  display.display();
}
