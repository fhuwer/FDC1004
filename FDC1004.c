#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <util/delay.h>
#include "FDC1004.h"
#include "i2cmaster.h"
#include <stdlib.h>

uint8_t FDC1004_MEAS_CONFIG[]    = { 0x08, 0x09, 0x0A, 0x0B };
uint8_t FDC1004_MEAS_MSB[]       = { 0x00, 0x02, 0x04, 0x06 };
uint8_t FDC1004_MEAS_LSB[]       = { 0x01, 0x03, 0x05, 0x07 };
uint8_t FDC1004_capdac_values[]  = { 0, 0, 0, 0 };

/**
 * @brief Initialize the connection to the FDC1004.
 * @details Initialize the connection to the FDC1004 via hardware TWI. To check
 * if the connection is working properly the manufacturer and device id are
 * checked against the value given in the datasheet.
 *
 * @return 0 if successful, 1 otherwise
 */
uint8_t FDC1004_init() {
   i2c_init();

   // Check manufacturer id
   if (FDC1004_read(FDC1004_REG_MANUFACTURER_ID) != FDC1004_MANUFACTURER_ID) {
      return 1;
   }

   // Check device id
   if (FDC1004_read(FDC1004_REG_DEVICE_ID) != FDC1004_DEVICE_ID) {
      return 1;
   }

   return 0;
}

/**
 * @brief Write data directly to a register in the FDC.
 *
 * @param reg Register to write to.
 * @param data 16-bit data to write.
 */
uint8_t FDC1004_write(uint8_t reg, uint16_t data) {
   i2c_start(FDC1004_ADDRESS + I2C_WRITE);
   i2c_write(reg);
   i2c_write((uint8_t)(data >> 8));
   uint8_t result = i2c_write(data);
   i2c_stop();
   return result;
}

/**
 * @brief Read a 16-bit register from the FDC.
 *
 * @param reg Register to read from
 * @return Data in the register
 */
uint16_t FDC1004_read(uint8_t reg) {
   // Write register address into register pointer
   i2c_start_wait(FDC1004_ADDRESS + I2C_WRITE);
   i2c_write(reg);

   // Actually read the data back
   uint16_t data = 0;
   i2c_rep_start(FDC1004_ADDRESS + I2C_READ);
   data  = ((uint16_t)i2c_readAck()) << 8;
   data |= i2c_readNak();
   i2c_stop();
   return data;
}

/**
 * @brief Configure a single measurement.
 * @details Write a measurement configuration to the FDC. A measurement
 * configuration consists out of the channel to use, the measurement mode and
 * the value for the CAPDAC.
 *
 * @param measurement Measurement ID (0 - 3)
 * @param channel Channel ID (0 - 3)
 * @param capdac Capacity offset (C_offset = capdac * 3.125 pF)
 * @return 0 if successful, 1 otherwise
 */
uint8_t FDC1004_configure_single_measurement(uint8_t measurement, uint8_t
      channel, uint8_t capdac) {
   /* Measurement configuration register:
         [15:13] = Positive channel number
         [12:10] = Negative channel number (or CAPDAC/DISABLE)
         [ 9:5 ] = CAPDAC 0b00000 - 0b11111 (C_offset = CAPDAC * 3.125 pF)
         [ 4:0 ] = RESERVED, always 0
   */
   uint16_t conf_data = ((uint16_t)channel) << 13;
   conf_data |= FDC1004_CAPDAC << 10;
   conf_data |= ((uint16_t)capdac) << 5;
   FDC1004_capdac_values[measurement] = capdac;
   if (FDC1004_write(FDC1004_MEAS_CONFIG[measurement], conf_data)) {
      // Writing the configuration failed.
      return 1;
   }
   return 0;
}

/**
 * @brief Configure a differential measurement.
 * @details Write a measurement configuration to the FDC. A measurement
 * configuration consists on the channel to use, the measurement mode and the
 * value for the CAPDAC.
 *
 * @param measurement Measurement ID (0 - 3)
 * @param channel Channel ID (0 - 3)
 * @param capdac Capacity offset (C_offset = capdac * 3.125 pF)
 * @return 0 if successful, 1 otherwise
 */
uint8_t FDC1004_configure_differential_measurement(uint8_t measurement, uint8_t
      channel_a, uint8_t channel_b) {
   /* Measurement configuration register:
         [15:13] = Positive channel number
         [12:10] = Negative channel number (or CAPDAC/DISABLE)
         [ 9:5 ] = CAPDAC 0b00000 - 0b11111 (C_offset = CAPDAC * 3.125 pF)
         [ 4:0 ] = RESERVED, always 0
   */
   uint16_t conf_data = ((uint16_t)channel_a) << 13;
   conf_data |= ((uint16_t)channel_b) << 10;
   conf_data |= ((uint16_t)0b00000) << 5;
   if (FDC1004_write(FDC1004_MEAS_CONFIG[measurement], conf_data)) {
      // Writing the configuration failed.
      return 1;
   }
   return 0;
}

/**
 * @brief Trigger a measurement.
 * @details Trigger a measurement. Note that the measurement is only triggered
 * here, it is not read out nor waits this function on the completion of the
 * measurement. For reading the measurement directly please use
 * FDC1004_measure_channel.
 *
 * @param measurement Measurement ID (0 - 3)
 * @return Success code
 */
uint8_t FDC1004_trigger_measurement(uint8_t measurement) {
   /* FDC configuration register:
         [ 15  ] = Reset
         [14:12] = RESERVED, always 0 (RO)
         [11:10] = Measurement rate
         [  9  ] = RESERVED, always 0 (RO)
         [  8  ] = Repeat
         [ 7:4 ] = Enable measurement 1-4
         [ 3:0 ] = Measurement done
   */
   uint16_t conf_data = 0;
   conf_data |= FDC1004_RATE << 10;
   conf_data |= ((uint16_t)0) << 8; // Dont repeat
   conf_data |= ((uint16_t)1) << (7 - measurement);
   FDC1004_write(FDC1004_REG_FDC, conf_data);
   return 0;
}

/**
 * @brief Read raw measurement result.
 */
uint8_t FDC1004_read_raw_measurement(uint8_t measurement, int32_t *result) {
   // Check if measurement is done
   uint16_t fdc_register = FDC1004_read(FDC1004_REG_FDC);
   if (!(fdc_register & (((uint16_t)1) << (3 - measurement)))) {
      return 1;
   }

   uint32_t msb = FDC1004_read(FDC1004_MEAS_MSB[measurement]);
   uint32_t lsb = FDC1004_read(FDC1004_MEAS_LSB[measurement]);

   /* Data format:
    *    MSB[15:0] - 16 MSB of measurement
    *    LSB[15:8] - 8 LSB of measurement
    *    LSB[ 7:0] - Reserved, always 0
    */
   (*result) = (msb << 8) | (lsb >> 8);

   // Convert Two's complement to signed integer (necessary since the data is shifted by 8)
   uint32_t raw_result = (msb << 16) | lsb;
   if (raw_result & ((uint32_t)1 << 31)) {
      (*result) |= ((uint32_t)0xFF << 24);
   }

   return 0;
}

/**
 * @brief Read a measurement result.
 * @details Read the result of a previously triggered measurement. This
 * will not wait for the measurement to be ready. If it is not ready yet,
 * it will exit with an error code.
 *
 * @param measurement Measurement ID (0 - 3)
 * @param result Result in pF
 *
 * @return Success code
 */
uint8_t FDC1004_read_measurement(uint8_t measurement, double *result) {
   int32_t data;
   if (FDC1004_read_raw_measurement(measurement, &data)) {
      return 1;
   }
   // Value near lower end of the range
   if (data <= FDC1004_LOWER_LIMIT) {
      return 2;
   }
   // Value near upper end of the range
   if (data >= FDC1004_UPPER_LIMIT) {
      return 3;
   }

   (*result) = (double)data / ((int32_t)1 << 19) + FDC1004_capdac_values[measurement] * 3.125;
   return 0;
}

/*
 * @brief Measure the capacity on a channel.
 * @details Measure the capacity on a specified channel. This will
 * override the configuration of the measurement with the same id
 * as the channel to measure.
 *
 * @param channel Channel ID
 * @param capdac Capacity offset
 * @param result Result from measurement.
 * @return Status code
 */
uint8_t FDC1004_measure_channel(uint8_t channel, double *result) {
   uint8_t error_code = 0;
   while (1) {
      FDC1004_configure_single_measurement(channel, channel, FDC1004_capdac_values[channel]);
      FDC1004_trigger_measurement(channel);

      // Wait until result is ready
      while (1) {
         error_code = FDC1004_read_measurement(channel, result);
         if (error_code != 1) {
            break;
         }
      }

      // Measurement done
      if (error_code == 0) {
         return 0;
      }

      // Capdac value is to high (capacity is below range)
      if (error_code == 2) {
         if (FDC1004_capdac_values[channel] > 0) {
            FDC1004_capdac_values[channel] -= 1;
         } else {
            return 1;
         }
      }

      // Capdac value is to low (capacity is over range)
      if (error_code == 3) {
         if (FDC1004_capdac_values[channel] < 31) {
            FDC1004_capdac_values[channel] += 1;
         } else {
            return 1;
         }
      }
   }
   return 0;
}
