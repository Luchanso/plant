#include <stdint.h>

#include "ds3231.h"
#include "i2c.h"

// Refer to DS3231 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf
#define DS3231_I2C_ADDRESS 0x68
#define DS3231_REGISTERS_TOTAL 18
#define DS3231_FIRST_TIME_REGISTER 0
#define DS3231_TIME_REGISTERS 7

static uint8_t convertFromBCD(uint8_t value) {
  return ((value & 0xF0) >> 4) * 10 + (value & 0xF);
}
static uint8_t convertToBCD(uint8_t value) {
  uint8_t tens = value / 10;
  return ((tens) << 4 | (value - tens * 10));
}

bool pmDS3231ReadTime(time *t){
  uint8_t timeData[DS3231_TIME_REGISTERS]; //Temporary storage for incoming data.

  if(!pmI2CRead(DS3231_I2C_ADDRESS, DS3231_FIRST_TIME_REGISTER, DS3231_TIME_REGISTERS, timeData))
    return false;

  t->seconds    = convertFromBCD(timeData[0]);
  t->minutes    = convertFromBCD(timeData[1]);
  t->hours      = convertFromBCD(timeData[2]);
  t->dayOfWeek  = timeData[3];
  t->dayOfMonth = convertFromBCD(timeData[4]);
  // Most significant byte in month is responsible for century overflow
  t->month      = convertFromBCD(timeData[5] & 0x7F);
  t->year       = convertFromBCD(timeData[6]);

  return true;
}

bool pmDS3231SetTime(const time *const t) {
  uint8_t timeData[DS3231_TIME_REGISTERS];

  timeData[0] = convertToBCD(t->seconds);
  timeData[1] = convertToBCD(t->minutes);
  timeData[2] = convertToBCD(t->hours);
  timeData[3] = convertToBCD(t->dayOfWeek);
  timeData[4] = convertToBCD(t->dayOfMonth);
  timeData[5] = convertToBCD(t->month);
  timeData[6] = convertToBCD(t->year);

  if(!pmI2CWrite(DS3231_I2C_ADDRESS, DS3231_FIRST_TIME_REGISTER, DS3231_TIME_REGISTERS, timeData))
    return false;

  return true;
}