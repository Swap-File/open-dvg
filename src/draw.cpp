#include <Arduino.h>

#include "dac.h"
#include "assets.h"

//draw settings

// input output_brightness from 0 to X is mapped from normal to bright
#define BRIGHT_OFF 400     // "0 volts", relative to reference
#define BRIGHT_NORMAL 2500 // fairly bright
#define BRIGHT_BRIGHT 3200 // super bright

#define NORMAL_SHIFT 1 // but we can control with Z axis
#define OFF_SHIFT 5    // smaller numbers == slower transits

#define OFF_OUTPUT_DWELL0 0 // time to sit beam on before starting a transit
#define OFF_OUTPUT_DWELL1 0 // time to sit before starting a transit
#define OFF_OUTPUT_DWELL2 0 // time to sit after finishing a transit

#define REST_X 2048 // wait in the center of the screen
#define REST_Y 2048

#define MAX_PTS 4000
static uint16_t rx_points;
static uint32_t points[MAX_PTS];

// x and y position are in 12-bit range
static uint16_t x_pos;
static uint16_t y_pos;

static void output_dwell(int count)
{
  for (int i = 0; i < count; i++)
    dac_append_xy(x_pos, y_pos);
}

static void output_line(int x1, int y1, const int bright_shift)
{
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

  if (x0 <= x1)
  {
    dx = x1 - x0;
    sx = 1;
  }
  else
  {
    dx = x0 - x1;
    sx = -1;
  }

  if (y0 <= y1)
  {
    dy = y1 - y0;
    sy = 1;
  }
  else
  {
    dy = y0 - y1;
    sy = -1;
  }

  int err = dx - dy;

  while (1)
  {
    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy)
    {
      err = err - dy;
      x0 += sx;
      x_pos = x_off + (x0 << bright_shift);
    }
    if (e2 < dx)
    {
      err = err + dx;
      y0 += sy;
      y_pos = y_off + (y0 << bright_shift);
    }
    dac_append_xy(x_pos, y_pos);
  }

  // ensure that we end up exactly where we want
 x_pos = x1_orig;
  y_pos = y1_orig;
 dac_append_xy(x_pos, y_pos);
}

static void output_brightness(uint16_t bright)
{
  static uint16_t last_bright;
  if (last_bright == bright)
    return;
  last_bright = bright;

  output_dwell(OFF_OUTPUT_DWELL0);

  // scale bright from OFF to BRIGHT
  if (bright > 64)
    bright = 64;

  int bright_scaled = BRIGHT_OFF;
  if (bright > 0)
    bright_scaled = BRIGHT_NORMAL + ((BRIGHT_BRIGHT - BRIGHT_NORMAL) * bright) / 64;

  dac_append_wz(bright_scaled, bright_scaled);

  
}

void inline render_pt(int x1, int y1, uint16_t bright)
{
  
  
  if (bright == 0)
  {
    output_brightness(0);
    output_dwell(OFF_OUTPUT_DWELL1);
    output_line(x1, y1, OFF_SHIFT);
    output_dwell(OFF_OUTPUT_DWELL2);
  }
  else
  {
    output_brightness(bright);
    output_line(x1, y1, NORMAL_SHIFT);
  }
}

void draw_append(int x, int y, unsigned bright)
{
  // store the 12-bits of x and y, as well as 6 bits of output_brightness
  // (three in X and three in Y)
  points[rx_points++] = (bright << 24) | (x << 12) | y << 0;
  if (rx_points >= MAX_PTS)
    rx_points--;
}

void draw_clear_scene(void)
{
  rx_points = 0;
  x_pos = REST_X;
  y_pos = REST_Y;
}

void draw_render_scene(void)
{
  dac_reset();
  for (unsigned i = 0; i < rx_points; i++)
  {
    const uint32_t pt = points[i];
    uint16_t x = (pt >> 12) & 0xFFF;
    uint16_t y = (pt >> 0) & 0xFFF;
    unsigned intensity = (pt >> 24) & 0x3F;

    render_pt(x, y, intensity);
  }

  // go to the center of the screen, turn the beam off
 render_pt(REST_X,REST_Y , 0);
}
