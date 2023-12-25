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
    uint16_t co2ppm;
    uint16_t humidity;
    uint16_t temperature;
  };
  scd_40(i2c_bus_controller *);

  /**
   * Perform connectivity check by querying SCD40 serial number and waiting for
   * any valid response.
   * @return true if SCD40 is available for requests over I2C bus.
   */
  bool available();

  /**
   * Set compensation pressure to make measurement more precise.
   * @return true if new pressure has been succesfuly sent to SCD40
   */
  bool set_compensation_pressure(const uint16_t &hPa);

  /**
   * Start standard periodic measurement. New result will be available every 5
   * seconds.
   * @return true if measurement has started
   */
  bool start_measurement();

  /**
   * Start low power periodic measurement. New result will be available every 30
   * seconds.
   * @return true if measurement has started
   */
  bool start_low_power_measurement();

  /**
   * Check for new data
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
