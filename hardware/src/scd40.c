#include "scd40.h"

/*
Refer to SCD4x Datasheet:
https://sensirion.com/media/documents/48C4B7FB/6426E14D/CD_DS_SCD40_SCD41_Datasheet_D1_052023.pdf
*/

#define SCD40_START_MEASUREMENT 0x21b1
#define SCD40_GET_MEASUREMENT   0xEC05
#define SCD40_STOP_MEASUREMENT  0x3F86


static uint8_t SCD40GenerateCRC(const uint8_t* data, uint8_t count) {
  uint8_t current_byte;
  uint8_t crc = 0xFF;
  uint8_t crc_bit;

  /* calculates 8-Bit checksum with given polynomial */
  for (current_byte = 0; current_byte < count; ++current_byte) {
    crc ^= (data[current_byte]);

    for (crc_bit = 8; crc_bit > 0; --crc_bit) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x31; 
      else
        crc = (crc << 1);
    }
  }

  return crc;
}