#include <Arduino.h>

#include "assets.h"
#include "dac.h"
#include "draw.h"

static uint32_t fps_time = 0;
static uint32_t frame_start_time = 0;

bool test_pattern_loaded;

// Maximum amount of time to wait for a frame (in ms).
#define MAXIMUM_SERIAL_MS 20

// Maximum amount of time to render a scene before switching to lower quality
#define MAXIMUM_DAC_MS 26

// protocol flags from advancemame
#define FLAG_COMPLETE 0x0
#define FLAG_RGB 0x1
#define FLAG_XY 0x2
#define FLAG_EXIT 0x7
#define FLAG_FRAME 0x4
#define FLAG_QUALITY 0x3

void setup() {
  Serial.begin(115200);   // USB is always 12 Mbit/sec
  Serial5.begin(115200);  // Debug

  dac_init();
  assets_test_pattern();
  test_pattern_loaded = true;
}

// return 1 when frame is complete, otherwise return 0
static int read_data() {
  static uint32_t cmd = 0;
  static int frame_offset = 0;
  static uint8_t brightness = 0;

  uint8_t c = Serial.read();

  cmd = cmd << 8 | c;
  frame_offset++;
  if (frame_offset < 4)
    return 0;

  frame_offset = 0;

  uint8_t header = (cmd >> 29) & 0b00000111;

  // common case first
  if (header == FLAG_XY) {
    uint32_t y = (cmd >> 0) & 0x3fff;
    uint32_t x = (cmd >> 14) & 0x3fff;

    // blanking flag
    if ((cmd >> 28) & 0x01)
      draw_pt(x, y, 0);
    else
      draw_pt(x, y, brightness);
  } else if (header == FLAG_RGB) {
    // pick the max brightness from r g and b
    brightness =
        max(max((cmd >> 0) & 0xFF, (cmd >> 8) & 0xFF), (cmd >> 16) & 0xFF);
  }

  else if (header == FLAG_FRAME) {
    //uint32_t frame_complexity = cmd & 0b0001111111111111111111111111111;
    //Serial5.print(frame_complexity);
    //Serial5.print(' ');
    //TODO: Use frame_complexity to adjust screen writing algorithms
    draw_start_frame();
    frame_start_time = millis();
  } else if (header == FLAG_QUALITY) {
    //uint32_t game_quality = cmd & 0b0001111111111111111111111111111;
    //Serial5.print(game_quality);
    //Serial5.print(' ');
    //TODO: Use game_quality to adjust screen writing algorithms
  } else if (header == FLAG_COMPLETE) {
    draw_end_frame();
    return 1;  // start rendering!
  }

  return 0;
}

void loop() {
  static int fps = 0;
  static uint32_t total_serial_time = 0;
  static uint32_t total_dac_time = 0;

  uint32_t serial_start_time = millis();

  if (Serial.available() || !test_pattern_loaded) {
    while (1) {
      if (Serial.available()) {
        test_pattern_loaded = false;
        if (read_data())
          break;
      }
      if (millis() - serial_start_time > MAXIMUM_SERIAL_MS) {
        if (test_pattern_loaded == false) {
          assets_test_pattern();
          test_pattern_loaded = true;
        }

        break;
      }
    }
  } else {
    frame_start_time = serial_start_time;
  }

  total_serial_time = total_serial_time + (millis() - frame_start_time);

  uint32_t dac_start_time = millis();
  dac_output();
  uint32_t dac_cycle_time = millis() - dac_start_time;
  total_dac_time += dac_cycle_time;
  fps++;

  if (dac_cycle_time > MAXIMUM_DAC_MS) {
    if (draw_quality(2))
      Serial5.println("Switching to fast drawing.");
  } else if (dac_cycle_time < MAXIMUM_DAC_MS / 2) {
    if (draw_quality(1))
      Serial5.println("Switching to quality drawing.");
  }

  if (millis() - fps_time > 1000) {
    Serial5.print(total_serial_time);
    Serial5.print("ms Serial ");
    Serial5.print(total_dac_time);
    Serial5.print("ms DAC ");
    Serial5.print(fps);
    Serial5.println(" FPS");
    fps = 0;
    total_serial_time = 0;
    total_dac_time = 0;
    fps_time = millis();
  }
}
