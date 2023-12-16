#include <stdlib.h>

#include "bme280.h"
#include "i2c.h"
#include "convert_util.h"

//debug only, remove later
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
Stand by time    [t_sb   = 0bXXX] - does not matter, as mode is forced, not normal

This device does not use SPI to access BME280, for this reason we do not care
about SPI settings
SPI 3 wire       [spi3w_en = 0bX]

Combining this into configuration register map and register description in
sections 5.3 and 5.4, we get following settings values:
0xF2 ctrl_hum  [reserved: bits 7-3][osrs_h: bits 2-0] = 0b00000001
0xF4 ctrl_meas [osrs_t: bits 7-5][osrs_p: bits 4-2][mode: bits 1 and 0] = 0b00100101(0x25)
0xF5 config    [t_sb: bits 7-5][filter: bits 4-2][reserved: 1 bit][spi3w_en] = 0b00000000

Сonsequently, it is required to write registers 0xF2 and 0xF4, register 0xF5 is
initialized to 0 by default after power on. HOWEVER, forced mode will
self-reset to [mode = 0b00] after single measurement, so it must be rewritten
every time we want to make a measurement. Moreover, for extra power savings
this device can shut off its power entirely, therefore, all settings could be
lost and must be re-applied before every measurement.
*/
#define BME280_CTRL_HUM_VALUE      0x01
#define BME280_CTRL_MEAS_VALUE     0x25
#define BME280_CTRL_HUM_REGISTER   0xF2
#define BME280_STATUS_REGISTER     0xF3
#define BME280_CTRL_MEAS_REGISTER  0xF4

/*
Status register contains two bits indicating status: 0 and 3. The rest is
reserved and isn't a valid data, mask it.
*/
#define BME280_STATUS_REGISTER_MASK 0x05

/*
Section 5.4.1 of the datasheet claims that reading chip ID register should
always return 0x60.
*/
#define BME280_CHIP_ID_REGISTER    0xD0
#define BME280_CHIP_ID             0x60

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
#define BME280_FIRST_DATA_REGISTER 0xF7
#define BME280_DATA_REGISTERS 8

/*
As mentioned in unit 4.2 of the datasheed, measured data is just a set of ADC
values, those aren't useful on it's own. To convert those raw values to a
meaningful values, we'll the code suggested by the Bosch at the unit 8.2 of
the datasheet. In order to use this formulas, it is required to read a lot of
calibration data. This structure is presented in unit 4.2.2, Table 16.
*/
struct BME280CalibrationData {
  int32_t  fineT;
  uint16_t T1;
  int16_t  T2;
  int16_t  T3;
  uint16_t P1;
  int16_t  P2;
  int16_t  P3;
  int16_t  P4;
  int16_t  P5;
  int16_t  P6;
  int16_t  P7;
  int16_t  P8;
  int16_t  P9;
  uint8_t  H1;
  int16_t  H2;
  uint8_t  H3;
  int16_t  H4;
  int16_t  H5;
  int8_t   H6;
  bool initialized;
};

#define BME280_FIRST_T_P_CALIBRATION_REGISTER 0x88
#define BME280_T_P_CALIBRATION_REGISTERS_SIZE 24

#define BME280_H1_CALIBRATION_REGISTER 0xA1

#define BME280_FIRST_H2_H6_CALIBRATION_REGISTER 0xE1
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
static int32_t BME280CompensateT(const int32_t *const adc_T, BME280CalibrationData * d) {
  int32_t var1, var2, T;
  /*
  It looks terrifying, but bitshift operators are used as makeshift multiply or
  divide operators by the powers of 2; i.e. x << 1 == x * 2; y >> 2 == y / 4.
  Giving the fact that dividing 32 bit integers on an 8 bit CPU is fairly
  expensive, this will work faster.

  Take a look at var1. If you squint your eyes really hard, you could see the
  following expression:
  var1 = ( (adc_T / 8 - d->T1 / 2) * d->T2) / 2048 )

  However, it's still uncertain what exactly it does, please ask
  Bosch Sensortec about the details.
  */
  var1=((((*adc_T)>>3)-((int32_t)d->T1<<1))*((int32_t)d->T2))>>11;
  var2=((((((*adc_T)>>4)-((int32_t)d->T1))*(((*adc_T)>>4)-((int32_t)d->T1)))>>12)*((int32_t)d->T3))>>14;
  d->fineT=var1+var2;
  T=(d->fineT*5+128)>>8;
  return T;
}

/**
 * Calculate pressure from raw ADC value
 * @param adc_P raw 20 bit ADC value from 0xF7...0xF9 registers
 * @param d struct with calibration data
 * @return pressure in Pa as unsigned 32 bit integer. Output value of "96386"
 * equals 96386 Pa = 963.86 hPa
*/
static uint32_t BME280CompensateP(const int32_t *const adc_P, const BME280CalibrationData *const d) { 
  int32_t var1, var2; 
  uint32_t p;
  var1=(((int32_t)d->fineT)>>1)-(int32_t)64000;
  var2=(((var1>>2)*(var1>>2))>>11)*((int32_t)d->P6);
  var2=var2+((var1*((int32_t)d->P5))<<1);
  var2=(var2>>2)+(((int32_t)d->P4)<<16);
  var1=(((d->P3*(((var1>>2)*(var1>>2))>>13))>>3)+((((int32_t)d->P2)*var1)>>1))>>18;
  var1=((((32768+var1))*((int32_t)d->P1))>>15);
  if(var1==0){return 0;} // avoid exception caused by division by zero
  p=(((uint32_t)(((int32_t)1048576)-(*adc_P))-(var2>>12)))*3125;
  if(p<0x80000000){p=(p<<1)/((uint32_t)var1);}
  else{p=(p/(uint32_t)var1)*2;}
  var1=(((int32_t)d->P9)*((int32_t)(((p>>3)*(p>>3))>>13)))>>12;
  var2=(((int32_t)(p>>2))*((int32_t)d->P8))>>13;
  p=(uint32_t)((int32_t)p+((var1+var2+d->P7)>>4));
  return p;
}

/**
 * Calculate pressure from raw ADC value
 * @param adc_H raw 16 bit ADC value from 0xFD...0xFE registers
 * @param d struct with calibration data
 * @return humidity in %RH as unsigned 32 bit integer in Q22.10 format
 * (22 integer and 10 fractional bits).
 * Output value of "47445" represents 47445/1024 = 46.333 %RH
*/
static uint32_t BME280CompensateH(const int32_t *const adc_H, const BME280CalibrationData *const d) {
  int32_t v_x1_u32r;
  v_x1_u32r=(d->fineT-((int32_t)76800));
  v_x1_u32r=((((((*adc_H)<<14)-(((int32_t)d->H4)<<20)-(((int32_t)d->H5)*v_x1_u32r))+((int32_t)16384))>>15)*(((((((v_x1_u32r*((int32_t)d->H6))>>10)*(((v_x1_u32r*((int32_t)d->H3))>>11)+((int32_t)32768)))>>10)+((int32_t)2097152))*((int32_t)d->H2)+8192)>>14));
  v_x1_u32r=(v_x1_u32r-(((((v_x1_u32r>>15)*(v_x1_u32r>>15))>>7)*((int32_t)d->H1))>>4));
  v_x1_u32r=(v_x1_u32r<0?0:v_x1_u32r);
  v_x1_u32r=(v_x1_u32r>419430400?419430400:v_x1_u32r);
  return(uint32_t)(v_x1_u32r>>12);
}

BME280Data *createBME280() {
  BME280Data *data = calloc(1, sizeof(BME280Data));
  if(!data)
    return 0;

  data->d = calloc(1, sizeof(BME280CalibrationData));
  if(!data->d) {
    free(data);
    return 0;
  }

  return data;
}

void destroyBME280(BME280Data *data) {
  free(data->d);
  free(data);
}

bool BME280IsAvailable() {
  uint8_t chipID = 0;
  if(!pmI2CRead(BME280_I2C_ADDRESS, BME280_CHIP_ID_REGISTER, 1, &chipID))
    return false;

  return chipID == BME280_CHIP_ID;
}

bool BME280StartMeasurement() {
  uint8_t value = BME280_CTRL_HUM_VALUE;
  if(!pmI2CWrite(BME280_I2C_ADDRESS, BME280_CTRL_HUM_REGISTER, 1, &value))
    return false;

  value = BME280_CTRL_MEAS_VALUE;
  if(!pmI2CWrite(BME280_I2C_ADDRESS, BME280_CTRL_MEAS_REGISTER, 1, &value))
    return false;

  return true;
}

bool BME280IsIdle() {
  uint8_t status = 0xFF;
  if(!pmI2CRead(BME280_I2C_ADDRESS, BME280_STATUS_REGISTER, 1, &status))
    return false;

  return !(status & BME280_STATUS_REGISTER_MASK);
}

bool BME280GetCalibrationData(BME280CalibrationData *d) {
  uint8_t dataBuffer[BME280_T_P_CALIBRATION_REGISTERS_SIZE];
  if(!pmI2CRead(BME280_I2C_ADDRESS,
                BME280_FIRST_T_P_CALIBRATION_REGISTER, 
                BME280_T_P_CALIBRATION_REGISTERS_SIZE, 
                dataBuffer))
    return false;

  uint8_t *iterator = dataBuffer;

  getInt16FromLEBuffer(&iterator, &d->T1);
  getInt16FromLEBuffer(&iterator, &d->T2);
  getInt16FromLEBuffer(&iterator, &d->T3);

  getInt16FromLEBuffer(&iterator, &d->P1);
  getInt16FromLEBuffer(&iterator, &d->P2);
  getInt16FromLEBuffer(&iterator, &d->P3);
  getInt16FromLEBuffer(&iterator, &d->P4);
  getInt16FromLEBuffer(&iterator, &d->P5);
  getInt16FromLEBuffer(&iterator, &d->P6);
  getInt16FromLEBuffer(&iterator, &d->P7);
  getInt16FromLEBuffer(&iterator, &d->P8);
  getInt16FromLEBuffer(&iterator, &d->P9);

  if(!pmI2CRead(BME280_I2C_ADDRESS,
                BME280_H1_CALIBRATION_REGISTER, 
                1,
                dataBuffer))
    return false;

  iterator = dataBuffer;
  getInt8FromBuffer(&iterator, &d->H1);


  if(!pmI2CRead(BME280_I2C_ADDRESS,
                BME280_FIRST_H2_H6_CALIBRATION_REGISTER, 
                BME280_H2_H6_CALIBRATION_REGISTERS_SIZE, 
                dataBuffer))
    return false;

  iterator = dataBuffer;
  getInt16FromLEBuffer(&iterator, &d->H2);
  getInt8FromBuffer(&iterator, &d->H3);

  // Special case: register 0xE5 contains two halves of different values.
  d->H4 = *(iterator++) << 4;
  d->H4 |= (*(iterator) & 0xF);

  d->H5 = (*(iterator + 1)) << 4;
  d->H5 |= (*(iterator)) >> 4;
  iterator += 2;  

  getInt8FromBuffer(&iterator, &d->H6);

  d->initialized = true;
  
  return true;
}

bool BME280GetData(BME280Data *data){
  uint8_t dataBuffer[BME280_DATA_REGISTERS];
  if(!pmI2CRead(BME280_I2C_ADDRESS,
                BME280_FIRST_DATA_REGISTER, 
                BME280_DATA_REGISTERS, 
                dataBuffer))
    return false;

  int32_t pressure = 0;
  pressure = dataBuffer[0]; // most significant byte of pressure
  /*
  It is possible to do it in place, but, to avoid unnecessary type casting do
  it in the iterative way. BTW, those P and T values are 20 bit long, hence
  the weird byte shifting
  */
  pressure <<= 8;
  pressure |= dataBuffer[1]; // somewhat significat byte of pressure
  pressure <<= 8;
  pressure |= dataBuffer[2]; // least significant byte of pressure
  pressure >>= 4;

  int32_t temperature = 0;
  temperature = dataBuffer[3]; // most significant byte of pressure
  temperature <<= 8;
  temperature |= dataBuffer[4];// somewhat significant byte of pressure
  temperature <<= 8;     
  temperature |= dataBuffer[5];// least significant byte of pressure
  temperature >>= 4;

  int32_t humidity = 0;
  humidity = dataBuffer[6]; // most significannt byte of humidity
  humidity <<= 8;
  humidity |= dataBuffer[7];// least significant byte of humidity

  //Raw values we're just obtained require some post-processing.
  
  //Calibration values are constant, we only need to read them once.
  if(!data->d->initialized)
    if(!BME280GetCalibrationData(data->d))
      return false;

  data->temperature = BME280CompensateT(&temperature, data->d);
  data->pressure = BME280CompensateP(&pressure, data->d);
  data->humidity = BME280CompensateH(&humidity, data->d);

  return true;
}
