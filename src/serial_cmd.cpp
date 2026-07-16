#include "serial_cmd.h"
#include "AudioTools.h"
#include "config.h"
#include "eq.h"
#include "dsp_stream.h"
#include "dsp_converter.h"
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
  if (param == "eq1" || param == "bass") return String(eq_state.db[0], 1);
  if (param == "eq2" || param == "mid") return String(eq_state.db[1], 1);
  if (param == "eq3" || param == "treble") return String(eq_state.db[2], 1);
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
  else if (param == "eq1" || param == "eq2" || param == "eq3" ||
           param == "bass" || param == "mid" || param == "treble")
  {
    int idx;
    if (param == "bass" || param == "eq1") idx = 0;
    else if (param == "mid" || param == "eq2") idx = 1;
    else idx = 2;
    float v = val.toFloat();
    if (v < -12.0f || v > 12.0f) return false;
    eq_pending_db[idx] = v;
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
  Serial.println("DISPLAY [mode]    - switch display: 0/dashboard, 1/vu");
  Serial.println("");
  Serial.println("params:");
  Serial.println("  limit          1000-32767");
  Serial.println("  eq             on/off");
  Serial.println("  eq1/bass       -12 to 12 dB (200Hz low shelf)");
  Serial.println("  eq2/mid        -12 to 12 dB (1kHz peaking)");
  Serial.println("  eq3/treble     -12 to 12 dB (8kHz high shelf)");
  Serial.println("  width          on/off");
  Serial.println("  width_val      1.0-3.0");
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
    Serial.printf("eq=%s 1:%.1f 2:%.1f 3:%.1f\n",
                  eq_enabled ? "on" : "off",
                  eq_state.db[0], eq_state.db[1], eq_state.db[2]);
    Serial.printf("width=%s val=%.2f\n",
                  width_enabled ? "on" : "off",
                  stereo_width_q8 / 256.0f);
    Serial.printf("underflow=%lu rate=%luHz\n", underflow_count, current_sample_rate);
    Serial.printf("dsp_cpu=%lusb=%d\n", dsp_cpu_us, dsp_block_bytes);
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
          "eq", "bass", "mid", "treble",
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

  // ---- DISPLAY ----
  if (line.startsWith("DISPLAY"))
  {
    String mode = line.substring(7);
    mode.trim();
    if (mode == "0" || mode == "dashboard")
    {
      oled_mode = 0;
      oled_dirty = true;
      Serial.println("OK dashboard");
    }
    else if (mode == "1" || mode == "vu")
    {
      oled_mode = 1;
      oled_dirty = true;
      Serial.println("OK vu meter");
    }
    else
    {
      Serial.print("OK current: ");
      Serial.println(oled_mode == 0 ? "dashboard" : "vu");
    }
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
