#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

/*
SCD40 CO2 sensor driver, uses PlantMonitor I2C abstraction layer.
*/

#include <stdbool.h>
#include <stdint.h>

#include "i2c.h"

class scd_40 : protected i2c_peripheral {

public:
  struct measurement_data {
    // CO2 parts per million, as-is
    uint16_t co2ppm;
    // Relative humidity, %
    uint8_t humidity;
    // Temperature / 100, C. I.e. 2346 = 23.46 C
    int16_t temperature;
  };
  scd_40(i2c_bus_controller *);

  /**
   * Perform connectivity check by querying SCD40 for compensation pressure and
   * accepting any valid response.
   * @return true if SCD40 is available for requests over I2C bus.
   */
  bool available();

  /**
   * Query SCD40 serial number and waiting for any valid response.
   * NOTE: serial number cannot be read while sensor is performing measurement.
   * @param[out] serial_number output of the command.
   * @return true if serial number is successfuly fetched.
   */
  bool get_serial_number(uint64_t &serial_number);

  /**
   * Set compensation pressure to make measurement more precise.
   * NOTE: quite inaccurate, regular measurement is preferable.
   * @return true if new pressure has been succesfuly sent to SCD40
   */
  bool set_compensation_pressure(const uint16_t &hPa);

  /**
   * Start standard periodic measurement. New result will be available every 5
   * seconds. NOTE: this command will fail if sensor is already performing
   * measurement.
   * @return true if measurement has started
   */
  bool start_measurement();

  /**
   * Start low power periodic measurement. New result will be available every 30
   * seconds. NOTE: this command will fail if sensor is already performing
   * measurement.
   * @return true if measurement has started
   */
  bool start_low_power_measurement();

  /**
   * Check for the new data.
   * @return true if SCD40 has measurement data to send
   */
  bool measurement_ready();

  /**
   * Stop current standard/low power periodic measurement.
   * @return true if SCD40 has stopped periodic measurement.
   */
  bool stop_measurement();

  /**
   * Get measurement data.
   * @param [out] data data to write result to.
   */
  bool get_data(measurement_data &data);
};
