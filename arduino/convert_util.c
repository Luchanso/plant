#include "convert_util.h"

uint8_t convertFromBCD(uint8_t value) {
  return ((value & 0xF0) >> 4) * 10 + (value & 0xF);
}

uint8_t convertToBCD(uint8_t value) {
  uint8_t tens = value / 10;
  return ((tens) << 4 | (value - tens * 10));
}

void getInt16FromLEBuffer(uint8_t **rawBuffer, void *destination) {
  *((uint16_t*)destination) = (*rawBuffer)[1];
  *((uint16_t*)destination) <<= 8;
  *((uint16_t*)destination) |= (*rawBuffer)[0];
  *rawBuffer += 2;
}

void getInt8FromBuffer(uint8_t **rawBuffer, void *destination) {
  (*(uint8_t*)destination) = (**rawBuffer);
  *rawBuffer += 1;
}