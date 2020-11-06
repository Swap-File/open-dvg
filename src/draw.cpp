#include <Arduino.h>

#include "assets.h"
#include "dac.h"

//draw settings
#define REST_X 2048  // wait in the center of the screen
#define REST_Y 2048
// input render_brightness from 0 to X is mapped from normal to bright
#define BRIGHT_OFF 400      // "0 volts", relative to reference
#define BRIGHT_NORMAL 2500  // fairly bright
#define BRIGHT_BRIGHT 3200  // super bright

#define OFF_SHIFT 5  // smaller numbers == slower transits
int normal_shift = 1;

bool draw_quality(int i) {
  if (normal_shift != i) {
    normal_shift = i;
    return true;
  }
  return false;
}

// x and y position are in 12-bit range
static int x_pos;
static int y_pos;

static void render_line(int x1, int y1, const int bright_shift) {
  int dx;
  int dy;
  int sx;
  int sy;

  const int x1_orig = x1;
  const int y1_orig = y1;

  int x_off = x1 & ((1 << bright_shift) - 1);
  int y_off = y1 & ((1 << bright_shift) - 1);
  x1 >>= bright_shift;
  y1 >>= bright_shift;
  int x0 = x_pos >> bright_shift;
  int y0 = y_pos >> bright_shift;

  if (x0 <= x1) {
    dx = x1 - x0;
    sx = 1;
  } else {
    dx = x0 - x1;
    sx = -1;
  }

  if (y0 <= y1) {
    dy = y1 - y0;
    sy = 1;
  } else {
    dy = y0 - y1;
    sy = -1;
  }

  int err = dx - dy;

  while (1) {
    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err = err - dy;
      x0 += sx;
      x_pos = x_off + (x0 << bright_shift);
    }
    if (e2 < dx) {
      err = err + dx;
      y0 += sy;
      y_pos = y_off + (y0 << bright_shift);
    }

    dac_append_xy(x_pos, y_pos);
  }

  // ensure that we end up exactly where we want
  if (x_pos != x1_orig || y_pos != y1_orig) {
    x_pos = x1_orig;
    y_pos = y1_orig;
    dac_append_xy(x_pos, y_pos);
  }
}

static void render_brightness(uint16_t bright) {
  static uint16_t last_bright;
  if (last_bright == bright)
    return;
  last_bright = bright;

  // scale bright from OFF to BRIGHT
  if (bright > 256)
    bright = 256;

  int bright_scaled = BRIGHT_OFF;
  if (bright > 0) {
    bright_scaled = BRIGHT_NORMAL + ((BRIGHT_BRIGHT - BRIGHT_NORMAL) * bright) / 256;
  }

  dac_append_wz(bright_scaled, bright_scaled);
}

void draw_pt(int x1, int y1, uint16_t bright) {
  render_brightness(bright);
  if (bright == 0) {
    render_line(x1, y1, OFF_SHIFT);
  } else {
    render_line(x1, y1, normal_shift);
  }
}

void draw_start_frame(void) {
  dac_buffer_reset();
  draw_pt(REST_X, REST_Y, 0);
}

void draw_end_frame(void) {
  draw_pt(REST_X, REST_Y, 0);
}
