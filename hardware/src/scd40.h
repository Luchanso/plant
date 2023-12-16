#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

/*
SCD40 CO2 sensor driver, uses PlantMonitor I2C abstraction layer.
*/

#include <stdbool.h>
#include <stdint.h>

/**
 * Perform connectivity check by querying SCD40 serial number and waiting for
 * any valid response.
 * @return true if SCD40 is available for requests over I2C bus.
*/
bool SCD40IsAvailable();

bool SCD40SetCompensationPressure(const uint16_t *const hPa);

bool SCD40StartMeasurement();

bool SCD40StartLowPowerMeasurement();

bool SCD40MeasurementReady();

bool SCD40StopMeasurement();

bool SCD40GetData(uint16_t *co2ppm, uint32_t *temperature, uint16_t *humidity);