
#include <Arduino.h>

void dac_output(void);
void dac_init(void);
void dac_append_wz(uint16_t w, uint16_t z);
void dac_append_xy(uint16_t x, uint16_t y);
void dac_brightness(uint16_t bright);
void dac_reset(void);