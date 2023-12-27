#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

/*
BME280 Combined humidity and pressure sensor driver, uses PlantMonitor I2C
abstraction layer.
*/

#include <etl/memory.h>
#include <stdbool.h>
#include <stdint.h>

#include "i2c.h"

class bme_280 : protected i2c_peripheral {
  // The secret sauce. Contains boring values for processing raw measurements.
  struct calibration_data;
  etl::unique_ptr<calibration_data> m_calibration_data;
  bool get_calibration_data();

public:
  struct measurement_data {
    // pressure, daPa
    uint16_t pressure;
    // temperature / 100
    int16_t temperature;
    // relative humidity, %
    uint8_t humidity;
  };

  bme_280(i2c_bus_controller *);
  ~bme_280() = default;

  /**
   * Check connectivity by querying Chip ID register and validating it.
   * @return true if BME280 is available for requests over I2C bus.
   */
  bool available();

  /**
   * Send command to start measurement. Measurement might take some amount of
   * time. To ensure data relevancy, check that the device is done measuring
   * using @ref bme_280::idle()
   * @return true if command successfuly sent
   */
  bool start_measurement();

  /**
   * While BME280 is performing measurement or moving data to output registers,
   * it will consider itself busy. This function will check that BME280 is not
   * doing any of that.
   * @return true if BME280 is in idle state.
   */
  bool idle();

  /**
   * Get measurement result from the BME280. NOTE: this is fairly expensive
   * operation in terms of processing time, and, giving the fact that BME280 is
   * configured to run in the most power efficient mode, there is no need to run
   * this function more than once per minute.
   * @param calibration special data type storing device calibration data.
   */
  bool get_data(bme_280::measurement_data &data);
};

