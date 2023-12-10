#include <stdint.h>

#include "ds3231.h"
#include "i2c.h"
#include "convert_util.h"

/*
Refer to DS3231 datasheet:
https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf

Address is defined as following on page 17 of the datasheet, quote:
"...address byte contains the 7-bit DS3231 address, which is 1101000..."
0b1101000 is the same as 0x68.
*/
#define DS3231_I2C_ADDRESS 0x68

/*
The address map of the DS3231 is presented at page 11 of the datasheet.
In the section "Clock and Calendar" it is also mentioned, that "The contents of
the time and calendar registers are in the binary-coded decimal(BCD) format."
*/
#define DS3231_REGISTERS_TOTAL 18

#define DS3231_FIRST_TIMEDATE_REGISTER 0
#define DS3231_TIMEDATE_REGISTERS 7

#define DS3231_AGING_OFFSET_REGISTER 0x10

bool DS3231IsAvailable() {
  uint8_t temp = 0;
  return pmI2CRead(DS3231_I2C_ADDRESS, DS3231_AGING_OFFSET_REGISTER, 1, &temp);
}

bool DS3231ReadTime(time *t){
  uint8_t timeData[DS3231_TIMEDATE_REGISTERS];

  //Read all time and date registers at once.
  if(!pmI2CRead(DS3231_I2C_ADDRESS, DS3231_FIRST_TIMEDATE_REGISTER,
                DS3231_TIMEDATE_REGISTERS, timeData))
    return false;

  t->seconds    = convertFromBCD(timeData[0]);
  t->minutes    = convertFromBCD(timeData[1]);
  t->hours      = convertFromBCD(timeData[2]);
  t->dayOfWeek  = timeData[3];
  t->dayOfMonth = convertFromBCD(timeData[4]);

  // Most significant byte here is responsible for century overflow, drop it.
  t->month      = convertFromBCD(timeData[5] & 0x7F);
  
  t->year       = convertFromBCD(timeData[6]);

  return true;
}

bool DS3231SetTime(const time *const t) {
  uint8_t timeData[DS3231_TIMEDATE_REGISTERS];

  timeData[0] = convertToBCD(t->seconds);
  timeData[1] = convertToBCD(t->minutes);
  timeData[2] = convertToBCD(t->hours);
  timeData[3] = t->dayOfWeek;
  timeData[4] = convertToBCD(t->dayOfMonth);
  timeData[5] = convertToBCD(t->month);
  timeData[6] = convertToBCD(t->year);

  if(!pmI2CWrite(DS3231_I2C_ADDRESS, DS3231_FIRST_TIMEDATE_REGISTER,
                 DS3231_TIMEDATE_REGISTERS, timeData))
    return false;

  return true;
}