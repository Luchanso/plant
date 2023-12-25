#include "convert_util.h"

uint8_t convertFromBCD(uint8_t value) {
  return ((value & 0xF0) >> 4) * 10 + (value & 0xF);
}

uint8_t convertToBCD(uint8_t value) {
  uint8_t tens = value / 10;
  return ((tens) << 4 | (value - tens * 10));
}
