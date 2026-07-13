#include "led.h"

static uint32_t last_underflow_count = 0;
static unsigned long last_underflow_blink = 0;
static bool underflow_blink_state = false;
static unsigned long last_strip_update = 0;

void led_init(Adafruit_NeoPixel &strip)
{
  strip.begin();
  strip.setBrightness(40);
  strip.show();
}

void led_update(Adafruit_NeoPixel &strip, uint32_t underflow_count)
{
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
}
