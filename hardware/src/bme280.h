#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

/*
BME280 Combined humidity and pressure sensor driver, uses PlantMonitor I2C
abstraction layer.
*/

#include <stdbool.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

// The secret sauce. Contains boring values for processing raw measurements.
typedef struct BME280CalibrationData BME280CalibrationData;

// The representation of a BME280.
typedef struct {
uint32_t pressure;
int32_t  temperature;
uint32_t humidity;
BME280CalibrationData * d;
} BME280Data;

/**
 * Initialize BME280 instance.
 * @return properly initialized BME280Data or nullptr on failure.
 */
BME280Data *createBME280();

/**
 * Deinitialize BME280 instance.
 * @param data instance to destroy
 */
void destroyBME280(BME280Data *data);

/**
 * Check connectivity by querying Chip ID register and validating it.
 * @return true if BME280 is available for requests over I2C bus.
 */
bool BME280IsAvailable();

/**
 * Send command to start measurement. Measurement might take some amount of
 * time. To ensure data relevancy, check that the device is done measuring
 * using @ref pmBME280IsIdle()
 */
bool BME280StartMeasurement();

/**
 * While BME280 is performing measurement, it will consider itself busy. This
 * function will check that BME280 is not performing any measurement.
 * @return true if BME280 is in idle state.
 */
bool BME280IsIdle();

/**
 * Get measurement result from the BME280. NOTE: this is fairly expensive
 * operation in terms of processing time, and, giving the fact that BME280 is
 * configured to run in the most power efficient mode, there is no need to run
 * this function more than once per minute.
 * @param calibration special data type storing device calibration data.
 */
bool BME280GetData(BME280Data *data);

#if defined(__cplusplus)
}
#endif