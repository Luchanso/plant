#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// DS3231 real-time clock driver, uses PlantMonitor I2C abstraction layer.

#include <stdbool.h>

#include "time_struct.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Check connectivity by querying Aging Offset register and waiting for any
 * valid response over I2C bus.
 * @return true if DS3231 is available for requests over I2C bus.
 */
bool DS3231IsAvailable();

/**
 * Read time from DS3231.
 * @param[out] t time structure to write result to.
 * @return true, if read is successful.
 */
bool DS3231ReadTime(time *t);

/**
 * Set time to DS3231.
 * @param[in] t time to set
 * @return true on success.
 */
bool DS3231SetTime(const time *const t);

/**
 * Read the entire memory of DS3231.
 * @param 
bool pmDS3231ReadStatus()
*/
#if defined(__cplusplus)
}
#endif