#include <Arduino.h>
#include "assets.h"
#include "dac.h"

// Maximum amount of time to wait for a frame (in ms).
#define TARGET_FRAME_TIME 25  // exactly 40 fps
//draw settings
#define REST_X 2048  // wait in the center of the screen
#define REST_Y 2048
// input render_brightness from 0 to X is mapped from normal to bright

#define BRIGHT_OFF 400      // "0 volts", relative to reference
#define BRIGHT_NORMAL 2500  // fairly bright
#define BRIGHT_BRIGHT 3200  // super bright

int bright_max = BRIGHT_BRIGHT;

#define OFF_SHIFT 32  // fastest transit speed my monitor can do
int normal_shift = 2;

void draw_quality(int i) {
  if (i > TARGET_FRAME_TIME) {
    normal_shift++;
    Serial5.print("Decreasing draw quality to: ");
    Serial5.println(normal_shift);
    bright_max = BRIGHT_BRIGHT;
  } else if (i < TARGET_FRAME_TIME / 2) {
    if (normal_shift > 2) {
      normal_shift--;
      Serial5.print("Increasing draw quality to: ");
      Serial5.println(normal_shift);
    } else {
      // slowly decrease the brightness when we have extremely high FPS
      // high FPS will make things too bright
      if (bright_max > ((BRIGHT_BRIGHT + BRIGHT_NORMAL) / 2)) {
        bright_max -= 2;
        Serial5.print("Decreasing brightness to: ");
        Serial5.println(bright_max);
      }
    }
  }
}

// x and y position are in 12-bit range
static int x_pos;
static int y_pos;

/*
static void render_off(int x1, int y1, const int bright_shift) {
  int dx = abs(x_pos - x1);
  int dy = abs(y_pos - y1);

  // approximation of transit length
  int length = (dx + dy) / bright_shift;
  x_pos = x1;
  y_pos = y1;
  for (int i = 0; i < length; i++)
    dac_append_xy(x_pos, y_pos);
}
*/

static void render_line(int x1, int y1, const int bright_shift) {
  //straight X render acceleration
  if (x_pos == x1) {
    if (y1 < y_pos) {
      y1 += bright_shift;
      while (y_pos > y1) {
        y_pos -= bright_shift;
        dac_append_xy(x_pos, y_pos);
      }
      y_pos = y1 - bright_shift;
      dac_append_xy(x_pos, y_pos);
    } else {
      y1 -= bright_shift;
      while (y_pos < y1) {
        y_pos += bright_shift;
        dac_append_xy(x_pos, y_pos);
      }
      y_pos = y1 + bright_shift;
      dac_append_xy(x_pos, y_pos);
    }

  }
  //straight Y render acceleration
  else if (y_pos == y1) {
    if (x1 < x_pos) {
      x1 += bright_shift;
      while (x_pos > x1) {
        x_pos -= bright_shift;
        dac_append_xy(x_pos, y_pos);
      }
      x_pos = x1 - bright_shift;
      dac_append_xy(x_pos, y_pos);
    } else {
      x1 -= bright_shift;
      while (x_pos < x1) {
        x_pos += bright_shift;
        dac_append_xy(x_pos, y_pos);
      }
      x_pos = x1 + bright_shift;
      dac_append_xy(x_pos, y_pos);
    }
  } else {
    int dx;
    int dy;
    int sx;
    int sy;
    int counter = bright_shift;

    if (x_pos <= x1) {
      dx = x1 - x_pos;
      sx = 1;
    } else {
      dx = x_pos - x1;
      sx = -1;
    }

    if (y_pos <= y1) {
      dy = y1 - y_pos;
      sy = 1;
    } else {
      dy = y_pos - y1;
      sy = -1;
    }

    int err = dx - dy;

    while (1) {
      if (x_pos == x1 && y_pos == y1) {
        if (counter != 0)
          dac_append_xy(x_pos, y_pos);
        break;
      }

      int e2 = 2 * err;
      if (e2 > -dy) {
        err = err - dy;
        x_pos += sx;
      }
      if (e2 < dx) {
        err = err + dx;
        y_pos += sy;
      }

      if (--counter == 0) {  // 0 check is faster
        dac_append_xy(x_pos, y_pos);
        counter = bright_shift;
      }
    }
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
    bright_scaled = BRIGHT_NORMAL + ((bright_max - BRIGHT_NORMAL) * bright) / 256;
  }

  dac_append_wz(bright_scaled, bright_scaled);
}

void draw_pt(int x1, int y1, uint16_t bright) {
  render_brightness(bright);
  if (bright == 0) {
    render_line(x1, y1, OFF_SHIFT);  //render_off is marginally faster, but can leave artificats.
  } else {
    render_line(x1, y1, normal_shift);
  }
}

void draw_start_frame(void) {
  dac_buffer_reset();
  dac_append_xy(REST_X, REST_Y);
  dac_append_wz(0, 0);
}

void draw_end_frame(void) {
  draw_pt(REST_X, REST_Y, 0);
}
