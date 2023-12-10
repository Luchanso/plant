#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

/*
Basic I2C (also known as TWI in Atmel manuals) abstraction layer for AtMega328p/PlantMonitor.
This code assumes there is only one master device and it's this one.
*/

// If device contains multiple I2C interfaces, you can pick another one here.
#define I2C_PORT_LABEL C
#define I2C_SCL_PIN 5
#define I2C_SDA_PIN 4

#define I2C_SCL_FREQ 400000

#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Initialize I2C.
 */
void pmI2CInit();

/**
 * Read data from I2C bus. Will do burst read if @p length is greater than 1.
 * @param[in] address device address.
 * @param[in] deviceRegister read register from the device.
 * If length is greater than 1, function will read multiple registers in a row.
 * @param[in] length amount of bytes to read.
 * @param[out] data buffer to read data to.
 * MUST be initialized and big enough to fit @p length bytes.
 * @return true if operation was successful.
 */
bool pmI2CRead(const uint8_t address, const uint8_t deviceRegister, const uint8_t length, uint8_t *data);

/**
 * Write data to I2C bus. Will do burst write if @p length is greater than 1.
 * @param address device address.
 * @param length amount of bytes in input buffer.
 * @param deviceRegister register to write on the device.
 * If length is greater than 1, function will write multiple registers in a row.
 * @param data buffer to read data from.
 * @return true if operation was successful.
 */
bool pmI2CWrite(const uint8_t address, const uint8_t deviceRegister, const uint8_t length, const uint8_t *data);

#if defined(__cplusplus)
}
#endif