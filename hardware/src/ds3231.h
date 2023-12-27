#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// DS3231 real-time clock driver, uses PlantMonitor I2C abstraction layer.

#include <stdbool.h>

#include "i2c.h"
#include "time_struct.h"

class ds_3231 : protected i2c_peripheral {
public:
  ds_3231(i2c_bus_controller *);

  /**
   * Check connectivity by querying Aging Offset Register and waiting for
   * any valid response over I2C bus.
   * @return true if DS3231 is available for requests over I2C bus.
   */
  bool available();

  /**
   * Get temperature from DS3231 clock sensor. Has the prescision of 0.25
   * degrees C.
   * @param[out] temperature value to write result to.
   * @return true if read is successful.
   */
  bool get_temperature(uint16_t &temperature);

  /**
   * Read time from DS3231.
   * @param[out] t time structure to write result to.
   * @return true, if read is successful.
   */
  bool get_time(time &t);

  /**
   * Set time to DS3231.
   * @param[in] t time to set
   * @return true on success.
   */
  bool set_time(const time &t);

  /*
  bool get_status();
  */
};
