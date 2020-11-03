#include <Arduino.h>
#include <SPI.h>

#define FLIP_X
#define FLIP_Y

// Physical Layout
#define PIN_XW_DAC_CS 6
#define PIN_YZ_DAC_CS 10
#define DAC_X_CHAN 1
#define DAC_Y_CHAN 0
#define DAC_W_CHAN 1
#define DAC_Z_CHAN 1

// SPI Port Memory Addresses for Direct Access
#define SPI_TCR 0x60
#define SPI_TDR 0x64
#define SPI_RSR 0x70
#define SPI_RDR 0x74
#define SPI0ADDR(x) (*(volatile unsigned long *)(0x403A0000 + x))
#define SPI1ADDR(x) (*(volatile unsigned long *)(0x4039C000 + x))

// Buffer for a full frame of data
#define DAC_MAX (4096 * 8)  //~20ms worth of DAC samples at 32mhz
uint16_t dac0[DAC_MAX];
uint16_t dac1[DAC_MAX];
int dac_ptr;

inline static uint16_t mpc4921_write(int channel, uint16_t value) {
  value &= 0x0FFF;  // mask out just the 12 bits of data
#if BUFFERED
  // select the output channel, buffered, no gain
  value |= 0x7000 | (channel == 1 ? 0x8000 : 0x0000);
#else
  // select the output channel, unbuffered, no gain
  value |= 0x3000 | (channel == 1 ? 0x8000 : 0x0000);
#endif
  return value;
}

static inline void dac_append(uint16_t buffer1, uint16_t buffer2) {
  dac0[dac_ptr] = buffer1;
  dac1[dac_ptr] = buffer2;
  dac_ptr++;
}

void dac_append_xy(uint16_t x, uint16_t y) {
#ifdef FLIP_X
  x = mpc4921_write(DAC_X_CHAN, 4095 - x);
#else
  x = mpc4921_write(DAC_X_CHAN, x);
#endif

#ifdef FLIP_Y
  y = mpc4921_write(DAC_Y_CHAN, 4095 - y);
#else
  y = mpc4921_write(DAC_Y_CHAN, y);
#endif

  dac_append(x, y);
}

void dac_append_wz(uint16_t w, uint16_t z) {
  w = mpc4921_write(DAC_W_CHAN, w);
  z = mpc4921_write(DAC_Z_CHAN, z);
  dac_append(w, z);
}

void dac_output(void) {
  volatile int temp;

  for (int i = 0; i < dac_ptr; i++) {
    SPI1ADDR(SPI_TDR) = dac0[i];
    digitalWriteFast(PIN_XW_DAC_CS, LOW);  // this will go low before x transmission starts
    SPI0ADDR(SPI_TDR) = dac1[i];
    digitalWriteFast(PIN_YZ_DAC_CS, LOW);  // this will go low before y transmission starts
    delayNanoseconds(1000);  //tune this value based on spi bus speed
    temp = SPI1ADDR(SPI_RDR);
    temp = SPI0ADDR(SPI_RDR);
    digitalWriteFast(PIN_XW_DAC_CS, HIGH);
    digitalWriteFast(PIN_YZ_DAC_CS, HIGH);
    
  }
}

void dac_init(void) {
  // initialize SPI:
  SPI.begin();
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  SPI1.begin();
  SPI1.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  // turn on 16 bit mode
  SPI0ADDR(SPI_TCR) = (SPI0ADDR(SPI_TCR) & 0xfffff000) | LPSPI_TCR_FRAMESZ(15);
  SPI1ADDR(SPI_TCR) = (SPI1ADDR(SPI_TCR) & 0xfffff000) | LPSPI_TCR_FRAMESZ(15);

  // set the slaveSelectPin as an output:

  pinMode(PIN_XW_DAC_CS, OUTPUT);
  pinMode(PIN_YZ_DAC_CS, OUTPUT);
  digitalWriteFast(PIN_XW_DAC_CS, HIGH);
  digitalWriteFast(PIN_YZ_DAC_CS, HIGH);
}

void dac_reset(void) {
  dac_ptr = 0;
}