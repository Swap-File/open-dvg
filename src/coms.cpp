#include <Arduino.h>
#include "draw.h"

// protocol flags from advancemame
#define FLAG_COMPLETE 0x0
#define FLAG_RGB 0x1
#define FLAG_XY 0x2
#define FLAG_EXIT 0x7
#define FLAG_FRAME 0x4
#define FLAG_QUALITY 0x3

// return 1 when frame is complete, otherwise return 0
int coms_read(void) {
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
    // uint32_t frame_complexity = cmd & 0b0001111111111111111111111111111;
    // total vector length in pixels
    // FLAG_FRAME does not differentiate between on and off vectors
    // so it's less useful than measuring actual render time
    draw_start_frame();
  } else if (header == FLAG_QUALITY) {
    // uint32_t game_quality = cmd & 0b0001111111111111111111111111111;
    // Serial5.print(game_quality);
    // Serial5.println(' ');
    // FLAG_QUALITY is a fixed number defined by someone per game
    // this could be used to change rendering speeds, but it has to be set in advance
  } else if (header == FLAG_COMPLETE) {
    draw_end_frame();
    return 1;  // start rendering!
  }

  return 0;
}