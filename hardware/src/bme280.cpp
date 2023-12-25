#include <etl/vector.h>
#include <stdlib.h>

#include "bme280.h"
#include "convert_util.h"
#include "i2c.h"

#include "usart.h"

/*
Refer to BME280 datasheet:
https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf

According to section 3.5.1, Plant monitor can be better described as weather
monitoring station. This means, the following settings are recommended:
Filter off, i.e. [filter = 0b000]
Forced mode, i.e.   [mode = 0b01]
Oversampling x1 for everything, i.e.
Pressure         [osrs_p = 0b001]
Temperature      [osrs_t = 0b001]
Humidity         [osrs_h = 0b001]
Stand by time    [t_sb   = 0bXXX] - does not matter, as mode is forced, not
normal

This device does not use SPI to access BME280, for this reason we do not care
about SPI settings
SPI 3 wire       [spi3w_en = 0bX]

Combining this into configuration register map and register description in
sections 5.3 and 5.4, we get following settings values:
0xF2 ctrl_hum  [reserved: bits 7-3][osrs_h: bits 2-0] = 0b00000001
0xF4 ctrl_meas [osrs_t: bits 7-5][osrs_p: bits 4-2][mode: bits 1 and 0] =
0b00100101(0x25) 0xF5 config    [t_sb: bits 7-5][filter: bits 4-2][reserved: 1
bit][spi3w_en] = 0b00000000

Сonsequently, it is required to write registers 0xF2 and 0xF4, register 0xF5 is
initialized to 0 by default after power on. HOWEVER, forced mode will
self-reset to [mode = 0b00] after single measurement, so it must be rewritten
every time we want to make a measurement. Moreover, for extra power savings
this device can shut off its power entirely, therefore, all settings could be
lost and must be re-applied before every measurement.
*/
#define BME280_CTRL_HUM_VALUE 0x01
#define BME280_CTRL_MEAS_VALUE 0x25
#define BME280_CTRL_HUM_REGISTER (uint8_t)0xF2
#define BME280_STATUS_REGISTER (uint8_t)0xF3
#define BME280_CTRL_MEAS_REGISTER (uint8_t)0xF4

/*
Status register contains two bits indicating status: 0 and 3. The rest is
reserved and isn't a valid data, mask it.
*/
#define BME280_STATUS_REGISTER_MASK 0x05

/*
Section 5.4.1 of the datasheet claims that reading chip ID register should
always return 0x60.
*/
#define BME280_CHIP_ID_REGISTER (uint8_t)0xD0
#define BME280_CHIP_ID 0x60

/*
Section 6.2 mentions the two possible I2C addresses, 0x76 if SDO pin is
connected to GND and 0x77, if SDO pin is connected to Vcc. Vcc pin is
connnected to GND on my BME280 breakout board.
*/
#define BME280_I2C_ADDRESS 0x76

/*
Registers containing raw ADC values for various parameters. Values make no sense
without proper conversion.
*/
#define BME280_FIRST_DATA_REGISTER (uint8_t)0xF7
#define BME280_DATA_REGISTERS 8

/*
As mentioned in unit 4.2 of the datasheed, measured data is just a set of ADC
values, those aren't useful on it's own. To convert those raw values to a
meaningful values, we'll the code suggested by the Bosch at the unit 8.2 of
the datasheet. In order to use this formulas, it is required to read a lot of
calibration data. This structure is presented in unit 4.2.2, Table 16.
*/
struct bme_280::calibration_data {
  int32_t fineT = 0;
  uint16_t T1 = 0;
  int16_t T2 = 0;
  int16_t T3 = 0;
  uint16_t P1 = 0;
  int16_t P2 = 0;
  int16_t P3 = 0;
  int16_t P4 = 0;
  int16_t P5 = 0;
  int16_t P6 = 0;
  int16_t P7 = 0;
  int16_t P8 = 0;
  int16_t P9 = 0;
  uint8_t H1 = 0;
  int16_t H2 = 0;
  uint8_t H3 = 0;
  int16_t H4 = 0;
  int16_t H5 = 0;
  int8_t H6 = 0;
  bool initialized = false;

  calibration_data() = default;
  int32_t compensate_temperature(const int32_t &adc_T);
  uint32_t compensate_pressure(const int32_t &adc_P);
  uint32_t compensate_humidity(const int32_t &adc_H);
};

#define BME280_FIRST_T_P_CALIBRATION_REGISTER (uint8_t)0x88
#define BME280_T_P_CALIBRATION_REGISTERS_SIZE 24

#define BME280_H1_CALIBRATION_REGISTER (uint8_t)0xA1

#define BME280_FIRST_H2_H6_CALIBRATION_REGISTER (uint8_t)0xE1
#define BME280_H2_H6_CALIBRATION_REGISTERS_SIZE 7

/*
A little reworked compensation formulas from unit 8.2 and 4.2.3 of the
datasheet; using alternative formulas, as AtMega328p is an 8-bit MCU without a
dedicated Floating Point Unit (FPU).

Don't blame or ask me, I have roughly zero clue how exactly this works.
*/

/**
 * Calculate temperature from raw ADC value
 * @param adc_T raw 20 bit ADC value from 0xFA...0xFC registers
 * @param d struct with calibration data
 * @return temperature in DegC, resolution is 0.01 DegC. Output value of "5123"
 * equals 51.23 DegC. d->fineT carries fine temperature for other compensation
 * formulas.
 */
int32_t
bme_280::calibration_data::compensate_temperature(const int32_t &adc_T) {
  int32_t var1, var2, T;
  /*
  It looks terrifying, but bitshift operators are used as makeshift multiply or
  divide operators by the powers of 2; i.e. x << 1 == x * 2; y >> 2 == y / 4.
  Giving the fact that dividing 32 bit integers on an 8 bit CPU is fairly
  expensive, this will work faster.

  Take a look at var1. If you squint your eyes really hard, you could see the
  following expression:
  var1 = ( (adc_T / 8 - T1 / 2) * T2) / 2048 )

  However, it's still uncertain what exactly it does, please ask
  Bosch Sensortec about the details.
  */
  var1 = (((adc_T >> 3) - ((int32_t)T1 << 1)) * (int32_t)T2) >> 11;

  // clang-format is definitely not happy with this expression :D
  var2 =
      (((((adc_T >> 4) - (int32_t)T1) * ((adc_T >> 4) - (int32_t)T1)) >> 12) *
       (int32_t)T3) >>
      14;

  fineT = var1 + var2;
  T = (fineT * 5 + 128) >> 8;
  return T;
}

/**
 * Calculate pressure from raw ADC value
 * @param adc_P raw 20 bit ADC value from 0xF7...0xF9 registers
 * @param d struct with calibration data
 * @return pressure in Pa as unsigned 32 bit integer. Output value of "96386"
 * equals 96386 Pa = 963.86 hPa
 */
uint32_t bme_280::calibration_data::compensate_pressure(const int32_t &adc_P) {
  int32_t var1, var2;
  uint32_t p;
  var1 = (((int32_t)fineT) >> 1) - (int32_t)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)P6);
  var2 = var2 + ((var1 * ((int32_t)P5)) << 1);
  var2 = (var2 >> 2) + (((int32_t)P4) << 16);
  var1 = (((P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
          ((((int32_t)P2) * var1) >> 1)) >>
         18;
  var1 = ((((32768 + var1)) * ((int32_t)P1)) >> 15);
  if (var1 == 0) {
    return 0;
  } // avoid exception caused by division by zero
  p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
  if (p < 0x80000000) {
    p = (p << 1) / ((uint32_t)var1);
  } else {
    p = (p / (uint32_t)var1) * 2;
  }
  var1 = (((int32_t)P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
  var2 = (((int32_t)(p >> 2)) * ((int32_t)P8)) >> 13;
  p = (uint32_t)((int32_t)p + ((var1 + var2 + P7) >> 4));
  return p;
}

/**
 * Calculate humidity from raw ADC value
 * @param adc_H raw 16 bit ADC value from 0xFD...0xFE registers
 * @param d struct with calibration data
 * @return humidity in %RH as unsigned 32 bit integer in Q22.10 format
 * (22 integer and 10 fractional bits).
 * Output value of "47445" represents 47445/1024 = 46.333 %RH
 */
uint32_t bme_280::calibration_data::compensate_humidity(const int32_t &adc_H) {
  int32_t v_x1_u32r;
  v_x1_u32r = (fineT - ((int32_t)76800));
  v_x1_u32r =
      (((((adc_H << 14) - (((int32_t)H4) << 20) - (((int32_t)H5) * v_x1_u32r)) +
         ((int32_t)16384)) >>
        15) *
       (((((((v_x1_u32r * ((int32_t)H6)) >> 10) *
            (((v_x1_u32r * ((int32_t)H3)) >> 11) + ((int32_t)32768))) >>
           10) +
          ((int32_t)2097152)) *
             ((int32_t)H2) +
         8192) >>
        14));
  v_x1_u32r =
      (v_x1_u32r -
       (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return (uint32_t)(v_x1_u32r >> 12);
}

bme_280::bme_280(i2c_bus_controller *controller)
    : i2c_peripheral(BME280_I2C_ADDRESS, controller),
      m_calibration_data(new bme_280::calibration_data) {}

bool bme_280::available() {
  etl::vector<uint8_t, 1> chipID(1);
  if (!read(BME280_CHIP_ID_REGISTER, chipID))
    return false;

  return chipID[0] == BME280_CHIP_ID;
}

bool bme_280::start_measurement() {
  const etl::vector<uint8_t, 1> hum_reg_value = {BME280_CTRL_HUM_VALUE};
  const etl::vector<uint8_t, 1> meas_reg_val = {BME280_CTRL_MEAS_VALUE};

  if (!write(BME280_CTRL_HUM_REGISTER, hum_reg_value))
    return false;

  if (!write(BME280_CTRL_MEAS_REGISTER, meas_reg_val))
    return false;

  return true;
}

bool bme_280::idle() {
  etl::vector<uint8_t, 1> status = {0xFF};
  if (!read(BME280_STATUS_REGISTER, status))
    return false;

  return !(status[0] & BME280_STATUS_REGISTER_MASK);
}

bool bme_280::get_calibration_data() {
  etl::vector<uint8_t, BME280_T_P_CALIBRATION_REGISTERS_SIZE> data_buffer(
      BME280_T_P_CALIBRATION_REGISTERS_SIZE);

  pmUSARTSendDebugText("querying data 1...");
  if (!read(BME280_FIRST_T_P_CALIBRATION_REGISTER, data_buffer))
    return false;

  uint8_t *iterator = data_buffer.begin();

  get_le(m_calibration_data->T1, iterator);
  get_le(m_calibration_data->T2, iterator);
  get_le(m_calibration_data->T3, iterator);

  get_le(m_calibration_data->P1, iterator);
  get_le(m_calibration_data->P2, iterator);
  get_le(m_calibration_data->P3, iterator);
  get_le(m_calibration_data->P4, iterator);
  get_le(m_calibration_data->P5, iterator);
  get_le(m_calibration_data->P6, iterator);
  get_le(m_calibration_data->P7, iterator);
  get_le(m_calibration_data->P8, iterator);
  get_le(m_calibration_data->P9, iterator);

  data_buffer.resize(1);
  if (!read(BME280_H1_CALIBRATION_REGISTER, data_buffer))
    return false;

  iterator = data_buffer.begin();
  get_le(m_calibration_data->H1, iterator);

  data_buffer.resize(BME280_H2_H6_CALIBRATION_REGISTERS_SIZE);
  if (!read(BME280_FIRST_H2_H6_CALIBRATION_REGISTER, data_buffer))
    return false;

  iterator = data_buffer.begin();
  get_le(m_calibration_data->H2, iterator);
  get_le(m_calibration_data->H3, iterator);

  // Special case: register 0xE5 contains two halves of different values.
  m_calibration_data->H4 = *(iterator++) << 4;
  m_calibration_data->H4 |= (*(iterator)&0xF);

  m_calibration_data->H5 = (*(iterator + 1)) << 4;
  m_calibration_data->H5 |= (*(iterator)) >> 4;
  iterator += 2;

  get_le(m_calibration_data->H6, iterator);

  m_calibration_data->initialized = true;

  return true;
}

bool bme_280::get_data(bme_280::measurement_data &data) {
  etl::vector<uint8_t, BME280_DATA_REGISTERS> data_buffer(
      BME280_DATA_REGISTERS);

  if (!read(BME280_FIRST_DATA_REGISTER, data_buffer))
    return false;

  int32_t pressure = 0;
  pressure = data_buffer[0]; // most significant byte of pressure
  /*
  It is possible to do it in place, but, to avoid unnecessary type casting do
  it in the iterative way. BTW, those P and T values are 20 bit long, hence
  the weird byte shifting
  */
  pressure <<= 8;
  pressure |= data_buffer[1]; // somewhat significat byte of pressure
  pressure <<= 8;
  pressure |= data_buffer[2]; // least significant byte of pressure
  pressure >>= 4;

  int32_t temperature = 0;
  temperature = data_buffer[3]; // most significant byte of pressure
  temperature <<= 8;
  temperature |= data_buffer[4]; // somewhat significant byte of pressure
  temperature <<= 8;
  temperature |= data_buffer[5]; // least significant byte of pressure
  temperature >>= 4;

  int32_t humidity = 0;
  humidity = data_buffer[6]; // most significannt byte of humidity
  humidity <<= 8;
  humidity |= data_buffer[7]; // least significant byte of humidity

  // Raw values we're just obtained require some post-processing.

  // Calibration values are constant, we only need to read them once.
  if (!m_calibration_data->initialized) {
    if (!get_calibration_data())
      return false;
    pmUSARTSendDebugText("Got data\r\n");
  }

  data.temperature = m_calibration_data->compensate_temperature(temperature);
  data.pressure = m_calibration_data->compensate_pressure(pressure);
  data.humidity = m_calibration_data->compensate_humidity(humidity);

  return true;
}
