#include "scd40.h"
#include "convert_util.h"

/*
Refer to SCD4x Datasheet:
https://sensirion.com/media/documents/48C4B7FB/6426E14D/CD_DS_SCD40_SCD41_Datasheet_D1_052023.pdf
*/

#define SCD40_I2C_ADDRESS (uint8_t)0x62

/*
...and I thought developers of BME280 were high on something, in a perspective,
it's understandable why they did what they did. But THIS! This is NOT how you
make an I2C peripheral! Why this device requires 16-bit register addresses, what
for? (keep in mind, thoose defines are in little endian).
*/
#define SCD40_START_MEASUREMENT (uint16_t)(0xB121)
#define SCD40_START_LOW_POWER_MEASUREMENT (uint16_t)(0xAC21)
#define SCD40_STOP_MEASUREMENT (uint16_t)(0x863F)

#define SCD40_MEASUREMENT_READY (uint16_t)(0xB8E4)
#define SCD40_MEASUREMENT_READY_RESP_SIZE (uint8_t)3
#define SCD40_MEASUREMENT_READY_MASK (uint16_t)(0x7FFF)

#define SCD40_GET_MEASUREMENT (uint16_t)(0x05EC)
#define SCD40_MEASUREMENT_RESP_SIZE (uint8_t)9

#define SCD40_COMPENSATION_PRESSURE (uint16_t)(0x00E0)
#define SCD40_COMPENSATION_PRESSURE_SIZE (uint8_t)3

#define SCD40_GET_SERIAL_NO (uint16_t)(0x8236)
#define SCD40_GET_SERIAL_NO_RESP_SIZE (uint8_t)9

// CRC after EACH! TWO! BYTES! OF DATA?! WHY!!?!?
static uint8_t scd_40_crc(const uint8_t *data, uint8_t count) {
  uint8_t current_byte;
  uint8_t crc = 0xFF;
  uint8_t crc_bit;

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

scd_40::scd_40(i2c_bus_controller *controller)
    : i2c_peripheral(SCD40_I2C_ADDRESS, controller) {
  /*
  SCD40 will begin measurement as soon as voltage settles after power on,
  this behaivor is undesirable. Stop measurement to prevent info readout block.
  */
  stop_measurement();
}

bool scd_40::available() {
  alloc_bytevect(pressure, SCD40_COMPENSATION_PRESSURE_SIZE);
  if (!read(SCD40_COMPENSATION_PRESSURE, pressure))
    return false;

  return !scd_40_crc(pressure.begin(), 3);
}

bool scd_40::get_serial_number(uint64_t &sn) {
  etl::vector<uint8_t, SCD40_GET_SERIAL_NO_RESP_SIZE> response(
      SCD40_GET_SERIAL_NO_RESP_SIZE);

  if (!read(SCD40_GET_SERIAL_NO, response))
    return false;

  // Serial number has 3 16-bit words, each word is followed by 8 bits of CRC
  uint8_t *iterator = response.begin();
  for (uint8_t i = 0; i < SCD40_GET_SERIAL_NO_RESP_SIZE; i += 3)
    // if sequence contains valid CRC in the end, function will return 0
    if (scd_40_crc(iterator + i, 3))
      return false;

  uint16_t word;
  sn = 0;
  for (uint8_t i = 0; i < 3; ++i) {
    get_be(word, iterator);
    sn <<= 16;
    sn |= word;
    ++iterator;
  }

  return true;
}

bool scd_40::set_compensation_pressure(const uint16_t &hPa) {
  alloc_bytevect(pressure, SCD40_COMPENSATION_PRESSURE_SIZE);
  uint8_t *iterator = pressure.begin();
  set_be(hPa, iterator);
  pressure[2] = scd_40_crc(iterator - 2, 2);

  return write(SCD40_COMPENSATION_PRESSURE, pressure);
}

bool scd_40::start_measurement() {
  etl::vector<uint8_t, 1> tmp;
  return write(SCD40_START_MEASUREMENT, tmp);
}

bool scd_40::start_low_power_measurement() {
  etl::vector<uint8_t, 1> tmp;
  return write(SCD40_START_LOW_POWER_MEASUREMENT, tmp);
}

bool scd_40::measurement_ready() {
  alloc_bytevect(response, SCD40_MEASUREMENT_READY_RESP_SIZE);

  if (!read(SCD40_MEASUREMENT_READY, response))
    return false;

  uint8_t *iterator = response.begin();
  if (scd_40_crc(iterator, 3))
    return false;

  uint16_t ready = 0;
  get_be(ready, iterator);

  // measurement is ready if any bit except the most significant is set
  return ready & SCD40_MEASUREMENT_READY_MASK;
}

bool scd_40::stop_measurement() {
  etl::vector<uint8_t, 1> tmp;
  return write(SCD40_STOP_MEASUREMENT, tmp);
}

bool scd_40::get_data(measurement_data &data) {
  alloc_bytevect(response, SCD40_MEASUREMENT_RESP_SIZE);
  if (!read(SCD40_GET_MEASUREMENT, response))
    return false;

  uint8_t *iterator = response.begin();
  for (uint8_t i = 0; i < SCD40_MEASUREMENT_RESP_SIZE; i += 3)
    if (scd_40_crc(iterator + i, 3))
      return false;

  get_be(data.co2ppm, iterator);
  ++iterator; // skip CRC byte
  get_be(data.temperature, iterator);
  ++iterator;

  uint16_t temp;
  get_be(temp, iterator);

  // CO2 ppm is usable as-is, temperature and humidity require some processing.
  data.temperature = (-4500 + (data.temperature / 3.74f));
  data.humidity = (temp / 655.35f);

  return true;
}
