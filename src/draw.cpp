#include <Arduino.h>

#include "dac.h"
#include "hershey_font.h"

//draw settings

// input brightness from 0 to X is mapped from normal to bright
#define BRIGHT_OFF 500      // "0 volts", relative to reference
#define BRIGHT_NORMAL 2500  // fairly bright
#define BRIGHT_BRIGHT 3200  // super bright

#define BRIGHT_SHIFT 2  // larger numbers == dimmer lines
#define NORMAL_SHIFT 2  // but we can control with Z axis

#define OFF_SHIFT 5  // smaller numbers == slower transits

#define OFF_DWELL0 0  // time to sit beam on before starting a transit
#define OFF_DWELL1 1  // time to sit before starting a transit
#define OFF_DWELL2 1  // time to sit after finishing a transit

#define REST_X 2048  // wait in the center of the screen
#define REST_Y 2048

#define MAX_PTS 3000
static unsigned rx_points;
static uint32_t points[MAX_PTS];

#define MOVETO (1 << 11)
#define LINETO (2 << 11)
#define BRIGHTTO (3 << 11)

// x and y position are in 12-bit range
static uint16_t x_pos;
static uint16_t y_pos;

void dwell(const int count) {
  for (int i = 0; i < count; i++) {
    dac_append_xy(x_pos, y_pos);
  }
}

static inline void _draw_lineto(int x1, int y1, const int bright_shift) {
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

  dac_append_xy(x_pos, y_pos);

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
    if (x0 == x1 && y0 == y1) break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err = err - dy;
      x0 += sx;
      uint16_t temp = x_off + (x0 << bright_shift);
      dac_append_xy(temp, y_pos);
      x_pos = temp;
    }
    if (e2 < dx) {
      err = err + dx;
      y0 += sy;
      uint16_t temp = y_off + (y0 << bright_shift);
      dac_append_xy(x_pos, temp);
      y_pos = temp;
    }
  }

  // ensure that we end up exactly where we want
  dac_append_xy(x1_orig, y1_orig);
  x_pos = x1_orig;
  y_pos = y1_orig;
}

void brightness(uint16_t bright) {
  static unsigned last_bright;
  if (last_bright == bright)
    return;
  last_bright = bright;

  dwell(OFF_DWELL0);

  // scale bright from OFF to BRIGHT
  if (bright > 64)
    bright = 64;

  int bright_scaled = BRIGHT_OFF;
  if (bright > 0)
    bright_scaled = BRIGHT_NORMAL + ((BRIGHT_BRIGHT - BRIGHT_NORMAL) * bright) / 64;

  dac_append_wz(bright_scaled,bright_scaled);
}


void draw_lineto(int x1, int y1, unsigned bright) {
  brightness(bright);
  _draw_lineto(x1, y1, NORMAL_SHIFT);
}

void draw_moveto(int x1, int y1) {
  brightness(0);

  // hold the current position for a few clocks
  // with the beam off

  dwell(OFF_DWELL1);

  _draw_lineto(x1, y1, OFF_SHIFT);

  dwell(OFF_DWELL2);
}
void draw_append(int x, int y, unsigned bright) {
  // store the 12-bits of x and y, as well as 6 bits of brightness
  // (three in X and three in Y)

  points[rx_points++] = (bright << 24) | (x << 12) | y << 0;
  if (rx_points >= MAX_PTS) {
    rx_points--;
  }
}

void moveto(int x, int y) {
  draw_append(x, y, 0);
}

void lineto(int x, int y) {
  draw_append(x, y, 24);  // normal brightness
}

void brightto(int x, int y) {
  draw_append(x, y, 63);  // max brightness
}

int draw_character(char c, int x, int y, int size) {
  const hershey_char_t *const f = &hershey_simplex[c - ' '];
  int next_moveto = 1;

  for (int i = 0; i < f->count; i++) {
    int dx = f->points[2 * i + 0];
    int dy = f->points[2 * i + 1];
    if (dx == -1) {
      next_moveto = 1;
      continue;
    }

    dx = (dx * size) * 3 / 4;
    dy = (dy * size) * 3 / 4;

    if (next_moveto)
      moveto(x + dx, y + dy);
    else
      lineto(x + dx, y + dy);

    next_moveto = 0;
  }

  return (f->width * size) * 3 / 4;
}

void draw_string(const char *s, int x, int y, int size) {
  while (*s) {
    char c = *s++;
    x += draw_character(c, x, y, size);
  }
}

void draw_new_scene(void) {
  rx_points = 0;
  x_pos = REST_X;
  y_pos = REST_Y;
}

void draw_test_pattern() {
  draw_new_scene();

  // fill in some points for test and calibration
  moveto(0, 0);
  lineto(1024, 0);
  lineto(1024, 1024);
  lineto(0, 1024);
  lineto(0, 0);

  // triangle
  moveto(4095, 0);
  lineto(4095 - 512, 0);
  lineto(4095 - 0, 512);
  lineto(4095, 0);

  // cross
  moveto(4095, 4095);
  lineto(4095 - 512, 4095);
  lineto(4095 - 512, 4095 - 512);
  lineto(4095, 4095 - 512);
  lineto(4095, 4095);

  moveto(0, 4095);
  lineto(512, 4095);
  lineto(0, 4095 - 512);
  lineto(512, 4095 - 512);
  lineto(0, 4095);

  moveto(2047, 2047 - 512);
  brightto(2047, 2047 + 512);

  moveto(2047 - 512, 2047);
  brightto(2047 + 512, 2047);

  // and a gradiant scale
  for (int i = 1; i < 63; i += 4) {
    moveto(1600, 2048 + i * 8);
    draw_append(1900, 2048 + i * 8, i);
  }

  // draw the sunburst pattern in the corner
  moveto(0, 0);
  for (unsigned j = 0, i = 0; j <= 1024; j += 128, i++) {
    if (i & 1) {
      moveto(1024, j);
      draw_append(0, 0, i * 7);
    } else {
      draw_append(1024, j, i * 7);
    }
  }

  moveto(0, 0);
  for (unsigned j = 0, i = 0; j < 1024; j += 128, i++) {
    if (i & 1) {
      moveto(j, 1024);
      draw_append(0, 0, i * 7);
    } else {
      draw_append(j, 1024, i * 7);
    }
  }

  draw_string("open-dvg", 2048 - 450, 2048 + 600, 6);

  draw_string("Firmware built", 2100, 1900, 3);
  draw_string(__DATE__, 2100, 1830, 3);
  draw_string(__TIME__, 2100, 1760, 3);

  int y = 2400;
  const int line_size = 70;

  draw_string("ELECTROHOME", 2100, y, 3);
  y -= line_size;
#ifdef FLIP_X
  draw_string("FLIP_X", 2100, y, 3);
  y -= line_size;
#endif
#ifdef FLIP_Y
  draw_string("FLIP_Y", 2100, y, 3);
  y -= line_size;
#endif
#ifdef SWAP_XY
  draw_string("SWAP_XY", 2100, y, 3);
  y -= line_size;
#endif
}

void draw_scene(void) {
  dac_reset();
  for (unsigned i = 0; i < rx_points; i++) {
    const uint32_t pt = points[i];
    uint16_t x = (pt >> 12) & 0xFFF;
    uint16_t y = (pt >> 0) & 0xFFF;
    unsigned intensity = (pt >> 24) & 0x3F;

    if (intensity == 0)
      draw_moveto(x, y);
    else
      draw_lineto(x, y, intensity);
  }

  dac_append_xy(REST_X, REST_Y);
}

