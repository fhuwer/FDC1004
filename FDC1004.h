#ifndef _FDC1004_H
#define _FDC1004_H

#include <stdint.h>

// 7 bit address is 0b101000
// here shifted it by one bit to include the R/W bit already
#define FDC1004_ADDRESS             0b10100000

// Lower and higher range limits (if lower/higher the capdac will be changed)
#define FDC1004_LOWER_LIMIT        -8388500
#define FDC1004_UPPER_LIMIT         8388500

// Values for enabling and disabling CAPDAC in configuration.
#define FDC1004_CAPDAC              0b100UL
#define FDC1004_DISABLED            0b111UL

// Measuring rate (0b01 = 100S/s, 0b010 = 200S/s, 0b11 = 400S/s)
#define FDC1004_RATE                0b01UL

// Communication rate for I2C-Bus (in Hz)
#define SCL_CLOCK                   1000000L

// Registers
#define FDC1004_REG_FDC              0x0C
#define FDC1004_REG_MANUFACTURER_ID  0xFE
#define FDC1004_REG_DEVICE_ID        0xFF

// Device properties
#define FDC1004_MANUFACTURER_ID      0x5449
#define FDC1004_DEVICE_ID            0x1004

extern uint8_t FDC1004_init();
extern uint8_t FDC1004_write(uint8_t reg, uint16_t data);
extern uint16_t FDC1004_read(uint8_t reg);
extern uint8_t FDC1004_configure_measurement(uint8_t measurement, uint8_t channel, uint8_t capdac);
extern uint8_t FDC1004_configure_differential_measurement(uint8_t measurement, uint8_t channel_a, uint8_t channel_b);
extern uint8_t FDC1004_trigger_measurement(uint8_t measurement);
extern uint8_t FDC1004_read_raw_measurement(uint8_t measurement, int32_t *result);
extern uint8_t FDC1004_read_measurement(uint8_t measurement, double *result);
extern uint8_t FDC1004_measure_channel(uint8_t channel, double *result);

#endif
