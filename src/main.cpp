#include <Arduino.h>
#include "dac.h"
#include "draw.h"
#include "assets.h"

static int fps = 0;
static uint32_t fps_time = 0;
static uint32_t frame_start_time = 0;

bool test_pattern_loaded;

// Maximum amount of time to wait for a frame (in ms).
#define MAXIMUM_SERIAL_MS 20

// protocol flags from advancemame
#define CMD_BUF_SIZE 0x20000
#define FLAG_COMPLETE 0x0
#define FLAG_RGB 0x1
#define FLAG_XY 0x2
#define FLAG_EXIT 0x7
#define FLAG_FRAME 0x4

void setup()
{
  Serial.begin(115200);  // USB is always 12 Mbit/sec
  Serial1.begin(115200); // Debug

  dac_init();
  draw_test_pattern();
  test_pattern_loaded = true;
}

// return 1 when frame is complete, otherwise return 0
static int read_data()
{
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
  if (header == FLAG_XY)
  {
    uint32_t y = (cmd >> 0) & 0x3fff;
    uint32_t x = (cmd >> 14) & 0x3fff;

    // blanking flag
    if ((cmd >> 28) & 0x01)
      draw_append(x, y, 0);
    else
      draw_append(x, y, brightness);
  }
  else if (header == FLAG_RGB)
  {
    // pick the max brightness from r g and b
    brightness =
        max(max((cmd >> 0) & 0xFF, (cmd >> 8) & 0xFF), (cmd >> 16) & 0xFF);
    // 8 bit to 6 bit brightness conversion
    brightness = brightness >> 2;
  }

  else if (header == FLAG_FRAME)
  {
    // uint32_t frame_complexity = cmd & 0b0001111111111111111111111111111;
    // Serial1.println(frame_complexity);
    // TODO: Use frame_complexity to adjust screen writing algorithms
    draw_clear_scene();
    frame_start_time = millis();
  }
  else if (header == FLAG_COMPLETE)
  {
    fps++;
    return 1; // start rendering!
  }

  return 0;
}

void loop()
{
  static uint32_t total_serial_time = 0;
  static uint32_t total_draw_time = 0;
  static uint32_t total_dac_time = 0;

  uint32_t serial_start_time = millis();

  if (Serial.available() || !test_pattern_loaded)
  {
    while (1)
    {
      if (Serial.available())
      {
        test_pattern_loaded = false;
        if (read_data())
          break;
      }
      if (millis() - serial_start_time > MAXIMUM_SERIAL_MS)
      {
        if (test_pattern_loaded == false)
        {
          draw_test_pattern();
          test_pattern_loaded = true;
        }
        break;
      }
    }
  }
  total_serial_time = total_serial_time + (millis() - serial_start_time);

  uint32_t draw_start_time = millis();
  draw_render_scene();
  total_draw_time = total_draw_time + (millis() - draw_start_time);

  uint32_t dac_start_time = millis();
  dac_output();
  total_dac_time = total_dac_time + (millis() - dac_start_time);

  if (millis() - fps_time > 1000)
  {
    Serial1.print((float)total_serial_time / 1000.0);
    Serial1.print("% Serial ");
    Serial1.print((float)total_draw_time / 1000.0);
    Serial1.print("% Render ");
    Serial1.print((float)total_dac_time / 1000.0);
    Serial1.print("% DAC ");
    Serial1.print(fps);
    Serial1.println(" FPS");
    fps = 0;
    total_serial_time = 0;
    total_draw_time = 0;
    total_dac_time = 0;
    fps_time = millis();
  }
}
