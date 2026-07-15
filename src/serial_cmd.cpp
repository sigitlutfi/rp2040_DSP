#include "serial_cmd.h"
#include "AudioTools.h"
#include "config.h"
#include "eq.h"
#include "dsp_stream.h"
#include "i2c_scanner.h"
#include "oled.h"

extern EQ3Band eq_state;
extern volatile int16_t stereo_width_q8;
extern volatile bool eq_needs_reinit;
extern volatile bool eq_pending_update;
extern volatile float eq_pending_db[3];
extern volatile bool eq_enabled;
extern volatile bool width_enabled;
extern volatile bool pre_buffered;
extern volatile bool streaming_active;

#define FIRMWARE_VERSION "1.0.0"

// ---- get param value as string ----
static String get_param(const String &param)
{
  if (param == "limit")
    return String(limiter_state.threshold);
  if (param == "eq")
    return eq_enabled ? "on" : "off";
  if (param == "eq_bass")
    return String(eq_state.db[0], 1);
  if (param == "eq_mid")
    return String(eq_state.db[1], 1);
  if (param == "eq_treble")
    return String(eq_state.db[2], 1);
  if (param == "width")
    return width_enabled ? "on" : "off";
  if (param == "width_val")
    return String(stereo_width_q8 / 256.0f, 2);
  return "";
}

// ---- set param ----
static bool set_param(const String &param, const String &val)
{
  if (param == "limit")
  {
    int v = val.toInt();
    if (v < 1000 || v > 32767) return false;
    limiter_state.threshold = v;
  }
  else if (param == "eq")
  {
    eq_enabled = (val == "on");
  }
  else if (param == "eq_bass")
  {
    float v = val.toFloat();
    if (v < -12.0f || v > 12.0f) return false;
    eq_pending_db[0] = v;
    eq_pending_update = true;
  }
  else if (param == "eq_mid")
  {
    float v = val.toFloat();
    if (v < -12.0f || v > 12.0f) return false;
    eq_pending_db[1] = v;
    eq_pending_update = true;
  }
  else if (param == "eq_treble")
  {
    float v = val.toFloat();
    if (v < -12.0f || v > 12.0f) return false;
    eq_pending_db[2] = v;
    eq_pending_update = true;
  }
  else if (param == "width")
  {
    width_enabled = (val == "on");
  }
  else if (param == "width_val")
  {
    float v = val.toFloat();
    if (v < 1.0f || v > 3.0f) return false;
    stereo_width_q8 = (int16_t)(v * 256);
  }
  else
  {
    return false;
  }
  oled_dirty = true;
  return true;
}

// ---- help text ----
static void print_help()
{
  Serial.println("=== RYNLABS USB Audio DSP ===");
  Serial.printf("  firmware: %s | rate: %luHz\n", FIRMWARE_VERSION, current_sample_rate);
  Serial.println("");
  Serial.println("PING              - connection test");
  Serial.println("VERSION           - firmware version");
  Serial.println("HELP              - this message");
  Serial.println("STATUS            - all params + stats");
  Serial.println("");
  Serial.println("GET ALL           - all param values");
  Serial.println("GET <param>       - single param value");
  Serial.println("SET <param> <val> - set param (OK/ERR)");
  Serial.println("SCAN              - scan I2C bus");
  Serial.println("RESET             - reset DSP stream (fix no-sound issue)");
  Serial.println("");
  Serial.println("params:");
  Serial.println("  limit        1000-32767");
  Serial.println("  eq           on/off");
  Serial.println("  eq_bass      -12 to 12 dB");
  Serial.println("  eq_mid       -12 to 12 dB");
  Serial.println("  eq_treble    -12 to 12 dB");
  Serial.println("  width        on/off");
  Serial.println("  width_val    1.0-3.0");
}

// ---- public API ----
void serial_cmd_init()
{
}

void serial_cmd_process()
{
  if (!Serial.available())
    return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0)
    return;

  // ---- PING ----
  if (line == "PING")
  {
    Serial.println("PONG");
    return;
  }

  // ---- VERSION ----
  if (line == "VERSION")
  {
    Serial.printf("VER %s\n", FIRMWARE_VERSION);
    return;
  }

  // ---- HELP ----
  if (line == "HELP" || line == "h" || line == "?")
  {
    print_help();
    return;
  }

  // ---- STATUS ----
  if (line == "STATUS" || line == "status")
  {
    Serial.printf("limit=%d\n", limiter_state.threshold);
    Serial.printf("eq=%s bass=%.1f mid=%.1f treble=%.1f\n",
                  eq_enabled ? "on" : "off",
                  eq_state.db[0], eq_state.db[1], eq_state.db[2]);
    Serial.printf("width=%s val=%.2f\n",
                  width_enabled ? "on" : "off",
                  stereo_width_q8 / 256.0f);
    Serial.printf("underflow=%lu rate=%luHz\n", underflow_count, current_sample_rate);
    return;
  }

  // ---- GET ----
  if (line.startsWith("GET "))
  {
    String param = line.substring(4);
    param.trim();
    if (param == "ALL")
    {
      const char *params[] = {
          "limit",
          "eq", "eq_bass", "eq_mid", "eq_treble",
          "width", "width_val"};
      for (int i = 0; i < 7; i++)
        Serial.printf("%s=%s%s", params[i], get_param(params[i]).c_str(),
                      i < 6 ? ";" : "\n");
      return;
    }
    String val = get_param(param);
    if (val.length() > 0)
      Serial.printf("%s=%s\n", param.c_str(), val.c_str());
    else
      Serial.println("ERR unknown param");
    return;
  }

  // ---- SET ----
  if (line.startsWith("SET "))
  {
    String rest = line.substring(4);
    rest.trim();
    int sp = rest.indexOf(' ');
    if (sp < 0)
    {
      Serial.println("ERR syntax: SET <param> <value>");
      return;
    }
    String param = rest.substring(0, sp);
    String val = rest.substring(sp + 1);
    param.trim();
    val.trim();
    if (set_param(param, val))
      Serial.println("OK");
    else
      Serial.println("ERR invalid param or range");
    return;
  }

  // ---- RESET ----
  if (line == "RESET" || line == "reset")
  {
    streaming_active = false;
    pre_buffered = false;
    Serial.println("OK DSP reset");
    return;
  }

  // ---- SCAN ----
  if (line == "SCAN" || line == "scan")
  {
    i2c_scanner_scan();
    return;
  }

  Serial.println("ERR unknown cmd (try HELP)");
}
