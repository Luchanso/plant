#include <stdint.h>

#include "convert_util.h"
#include "ds3231.h"
#include "i2c.h"

/*
Refer to DS3231 datasheet:
https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf

Address is defined as following on page 17 of the datasheet, quote:
"...address byte contains the 7-bit DS3231 address, which is 1101000..."
0b1101000 is the same as 0x68.
*/
#define DS3231_I2C_ADDRESS (uint8_t)0x68

/*
The address map of the DS3231 is presented at page 11 of the datasheet.
In the section "Clock and Calendar" it is also mentioned, that "The contents of
the time and calendar registers are in the binary-coded decimal(BCD) format."
*/
#define DS3231_REGISTERS_TOTAL (uint8_t)18

#define DS3231_FIRST_TIMEDATE_REGISTER (uint8_t)0
#define DS3231_TIMEDATE_REGISTERS (uint8_t)7

#define DS3231_AGING_OFFSET_REGISTER (uint8_t)0x10

ds_3231::ds_3231(i2c_bus_controller *controller)
    : i2c_peripheral(DS3231_I2C_ADDRESS, controller) {}

bool ds_3231::available() {
  etl::vector<uint8_t, 1> tmp(1);
  return read(DS3231_AGING_OFFSET_REGISTER, tmp);
}

bool ds_3231::get_time(time &t) {
  etl::vector<uint8_t, DS3231_TIMEDATE_REGISTERS> time_data(
      DS3231_TIMEDATE_REGISTERS);

  // Read all time and date registers at once.
  if (!read(DS3231_FIRST_TIMEDATE_REGISTER, time_data))
    return false;

  t.seconds = convertFromBCD(time_data[0]);
  t.minutes = convertFromBCD(time_data[1]);
  t.hours = convertFromBCD(time_data[2]);
  t.dayOfWeek = time_data[3];
  t.dayOfMonth = convertFromBCD(time_data[4]);

  // Most significant byte here is responsible for century overflow, drop it.
  t.month = convertFromBCD(time_data[5] & 0x7F);

  t.year = convertFromBCD(time_data[6]);

  return true;
}

bool ds_3231::set_time(const time &t) {
  etl::vector<uint8_t, DS3231_TIMEDATE_REGISTERS> time_data(
      DS3231_TIMEDATE_REGISTERS);

  time_data[0] = convertToBCD(t.seconds);
  time_data[1] = convertToBCD(t.minutes);
  time_data[2] = convertToBCD(t.hours);
  time_data[3] = t.dayOfWeek;
  time_data[4] = convertToBCD(t.dayOfMonth);
  time_data[5] = convertToBCD(t.month);
  time_data[6] = convertToBCD(t.year);

  if (!write(DS3231_FIRST_TIMEDATE_REGISTER, time_data))
    return false;

  return true;
}
