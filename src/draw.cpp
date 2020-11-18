#include <Arduino.h>
#include "assets.h"
#include "dac.h"

#define TARGET_FRAME_TIME 25  // exactly 40 fps

#define REST_X 2048  // wait in the center of the screen
#define REST_Y 2048

#define BRIGHTNESS_OFF 400      // off
#define BRIGHTNESS_NORMAL 2500  // fairly bright
#define BRIGHTNESS_BRIGHT 3200  // super bright
static int brightness_max = BRIGHTNESS_BRIGHT;

#define PIPIXEL_SKIP_VISIBLE_LINES 2
#define PIXEL_SKIP_TRANSIT_LINES 32  // fastest transit speed my monitor can do
static int pixel_skip = PIPIXEL_SKIP_VISIBLE_LINES;

// x and y position are in 12-bit range, but using an int is faster for math
static int x_pos;
static int y_pos;

void draw_quality(int i) {
  if (i > TARGET_FRAME_TIME) {
    pixel_skip++;
    Serial5.print("Increasing pixel skip to: ");
    Serial5.println(pixel_skip);
    brightness_max = BRIGHTNESS_BRIGHT;
  } else if (i < TARGET_FRAME_TIME / 2) {
    if (pixel_skip > 2) {
      pixel_skip--;
      Serial5.print("Decreasing pixel skip to: ");
      Serial5.println(pixel_skip);
    } else {
      // slowly decrease the brightness when we have extremely high FPS
      // high FPS will make things too bright
      if (brightness_max > ((BRIGHTNESS_BRIGHT + BRIGHTNESS_NORMAL) / 2)) {
        brightness_max -= 2;
        Serial5.print("Decreasing brightness to: ");
        Serial5.println(brightness_max);
      }
    }
  }
}

/*
static void render_off(int x1, int y1, const int pixel_skip) {
  int dx = abs(x_pos - x1);
  int dy = abs(y_pos - y1);

  // approximation of transit length
  int length = (dx + dy) / pixel_skip;
  x_pos = x1;
  y_pos = y1;
  for (int i = 0; i < length; i++)
    dac_append_xy(x_pos, y_pos);
}
*/

static void render_line(int x1, int y1, const int pixel_skip) {
  //straight X render acceleration
  if (x_pos == x1) {
    if (y1 < y_pos) {
      y1 += pixel_skip;
      while (y_pos > y1) {
        y_pos -= pixel_skip;
        dac_append_xy(x_pos, y_pos);
      }
      y_pos = y1 - pixel_skip;
      dac_append_xy(x_pos, y_pos);
    } else {
      y1 -= pixel_skip;
      while (y_pos < y1) {
        y_pos += pixel_skip;
        dac_append_xy(x_pos, y_pos);
      }
      y_pos = y1 + pixel_skip;
      dac_append_xy(x_pos, y_pos);
    }

  }
  //straight Y render acceleration
  else if (y_pos == y1) {
    if (x1 < x_pos) {
      x1 += pixel_skip;
      while (x_pos > x1) {
        x_pos -= pixel_skip;
        dac_append_xy(x_pos, y_pos);
      }
      x_pos = x1 - pixel_skip;
      dac_append_xy(x_pos, y_pos);
    } else {
      x1 -= pixel_skip;
      while (x_pos < x1) {
        x_pos += pixel_skip;
        dac_append_xy(x_pos, y_pos);
      }
      x_pos = x1 + pixel_skip;
      dac_append_xy(x_pos, y_pos);
    }
  } else {
    int dx;
    int dy;
    int sx;
    int sy;
    int counter = pixel_skip;

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
        counter = pixel_skip;
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

  int brightness_scaled = BRIGHTNESS_OFF;
  if (bright > 0) {
    brightness_scaled = BRIGHTNESS_NORMAL + ((brightness_max - BRIGHTNESS_NORMAL) * bright) / 256;
  }

  dac_append_wz(brightness_scaled, brightness_scaled);
}

void draw_pt(int x1, int y1, uint16_t bright) {
  render_brightness(bright);
  if (bright == 0) {
    render_line(x1, y1, PIXEL_SKIP_TRANSIT_LINES);  // render_off is marginally faster, but can leave artificats.
  } else {
    render_line(x1, y1, pixel_skip);
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
