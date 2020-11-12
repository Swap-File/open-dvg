#include <Arduino.h>

#include "assets.h"
#include "coms.h"
#include "dac.h"
#include "draw.h"

// how long in milliseconds to wait for data before displaying a test pattern
#define SERIAL_WAIT_TIME 20

void setup() {
  Serial.begin(115200);   // USB is always 12 Mbit/sec
  Serial5.begin(115200);  // Debug
  dac_init();
}

void loop() {
  static bool test_pattern_enabled = true;
  static int fps = 0;
  static uint32_t fps_time = 0;
  static uint32_t total_draw_time = 0;
  static uint32_t total_dac_time = 0;

  uint32_t draw_start_time = millis();

  while (1) {
    if (Serial.available()) {
      test_pattern_enabled = false;
      if (coms_read())
        break;
    }
    if (millis() - draw_start_time > SERIAL_WAIT_TIME) {
      Serial5.println("Switching to test pattern...");
      test_pattern_enabled = true;
    }
    if (test_pattern_enabled)
      break;
  }

  if (test_pattern_enabled)
    assets_test_pattern();

  total_draw_time += (millis() - draw_start_time);

  // output the dac buffer
  uint32_t dac_start_time = millis();

  dac_output();

  fps++;
  int dac_cycle_time = millis() - dac_start_time;
  total_dac_time += dac_cycle_time;
 
  draw_quality(dac_cycle_time);

  // occasionally print out stats for humans
  if (millis() - fps_time > 1000) {
    Serial5.print((float)total_draw_time / fps);
    Serial5.print(" draw  ");
    Serial5.print((float)total_dac_time / fps);
    Serial5.print(" dac  ");
    Serial5.print(total_draw_time);
    Serial5.print("ms draw  ");
    Serial5.print(total_dac_time);
    Serial5.print("ms dac  ");
    Serial5.print(fps);
    Serial5.println(" FPS");
    fps = 0;
    total_draw_time = 0;
    total_dac_time = 0;
    fps_time = millis();
  }
}
