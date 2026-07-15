#include "i2c_scanner.h"
#include "Wire.h"
#include "config.h"

void i2c_scanner_init()
{
  Wire.begin();
}

void i2c_scanner_scan()
{
  Serial.println("Scanning I2C bus (Wire: SDA=4, SCL=5)...");
  int count = 0;
  for (byte addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    byte err = Wire.endTransmission();
    if (err == 0)
    {
      Serial.printf("  Found: 0x%02X", addr);
      if (addr == 0x3C) Serial.print(" (SSD1306 OLED)");
      if (addr == 0x3D) Serial.print(" (SSD1306 OLED alt)");
      if (addr == 0x27) Serial.print(" (PCF8574 LCD)");
      if (addr == 0x3F) Serial.print(" (PCF8574 LCD alt)");
      Serial.println();
      count++;
    }
  }
  if (count == 0)
    Serial.println("  No I2C devices found");
  else
    Serial.printf("  Total: %d device(s)\n", count);
}
