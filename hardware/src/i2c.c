#include <util/twi.h>

#include "i2c.h"

/*
Refer to Atmega328p datasheet, section 21:
https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
*/

#define CONCATENATE(A, B) A ## B
#define CONCATENATE3(A, B, C) A ## B ## C
#define EVALUATE(A, B) CONCATENATE(A, B)
#define EVALUATE3(A, B, C) CONCATENATE3(A, B, C)

#define I2C_PORT  EVALUATE(PORT, I2C_PORT_LABEL)
#define I2C_DDR   EVALUATE(DDR,  I2C_PORT_LABEL)
#define I2C_SCL_P EVALUATE3(PORT, I2C_PORT_LABEL, I2C_SCL_PIN)
#define I2C_SDA_P EVALUATE3(PORT, I2C_PORT_LABEL, I2C_SDA_PIN)

// Note, for most common I2C frequencies using prescaler is not required.
#define I2C_PRESCALER 1

/*
Solving the formula provided by Atmel in Unit 21.5.2 of datasheet for TWBR:
SCL frequency = CPU frequency / (16 + 2 * TWBR * Prescaler value)
we'll get:
*/
#define I2C_TWBR ((F_CPU/I2C_SCL_FREQ - 16) / 2 / I2C_PRESCALER)

void pmI2CInit() {
  // Pull I2C pins high and leave them to I2C peripheral
  I2C_DDR  |=  (1 << I2C_SDA_P)  | (1 << I2C_SCL_P);
  I2C_PORT |=  (1 << I2C_SDA_P)  | (1 << I2C_SCL_P);
  I2C_DDR  &= ~((1 << I2C_SDA_P) | (1 << I2C_SCL_P));

  // We don't usually need a prescaler, set it to 1
  TWSR &= ~(TWPS0 | TWPS1);

  TWBR = I2C_TWBR;
}

static bool pmI2CStart() {
  // Send START condition
	TWCR =  (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
	
	// Wait for TWINT flag to set
	while (!(TWCR & (1 << TWINT)));
	
	// Check error
	if (TW_STATUS != TW_START && TW_STATUS != TW_REP_START)
		return false;
	
	return true;
}

static void pmI2CStop() {
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

static bool pmI2CSetAddress(uint8_t address, bool write) {
  TWDR = (address << 1) | (uint8_t)(write ? 0 : 1);
	TWCR = (1 << TWINT) | (1 << TWEN);

  //Wait for TWINT flag to set
	while (!(TWCR & (1 << TWINT)));

	if (!(TW_STATUS == TW_MT_SLA_ACK || TW_STATUS == TW_MR_SLA_ACK))
		return false;

	return true;
}

static bool pmI2CWriteByte(const uint8_t byte) {
  // Transmit 1 byte
  TWDR = byte;
  TWCR = (1 << TWINT) | (1 << TWEN);

	// Wait for TWINT flag to set
	while (!(TWCR & (1 << TWINT)));

  // Wait for ACK
	if (TW_STATUS != TW_MT_DATA_ACK)
    return false;

	return true;
}

static bool pmI2CReadByte(uint8_t *byte, bool sendAck) {
  TWCR = (1 << TWINT) | (1 << TWEN) | (sendAck ? (1 << TWEA) : 0);

  // Wait for operation to complete
  while (!(TWCR & (1 << TWINT)));

  if(sendAck) {
	  if (TW_STATUS != TW_MR_DATA_ACK)
	  	return false;
  } else {
    if (TW_STATUS != TW_MR_DATA_NACK)
      return false;
  }

  *byte = TWDR;
  return true;
}

bool pmI2CRead(const uint8_t address, const uint8_t deviceRegister, const uint8_t length, uint8_t *data) {
  if(!length)
    return false;

  // Send START condition
  if(!pmI2CStart())
    return false;

  if(!pmI2CSetAddress(address, true))
    return false;

  if(!pmI2CWriteByte(deviceRegister))
    return false;

  if(!pmI2CStart())
    return false;

  if(!pmI2CSetAddress(address, false))
    return false;

	// Read single or multiple data byte and send ack
	for (uint8_t i = 0; i < length - 1; ++i)
		if(!pmI2CReadByte(data + i, true))
      return false;

  if(!pmI2CReadByte(data - 1 + length, false))
    return false;

	// Send STOP condition
	pmI2CStop();
	
	return true;
}

bool pmI2CWrite(const uint8_t address, const uint8_t deviceRegister, const uint8_t length, const uint8_t *data) {
  if(!length)
    return false;

	// Send START condition
	if (!pmI2CStart())
		return false;
	
	// Send address with WRITE flag
	if (!pmI2CSetAddress(address, true))
    return false;
	
  // Set the first register to write to
  if(!pmI2CWriteByte(deviceRegister))
    return false;

  // Send bytes
	for (int i = 0; i < length; ++i)
		if (!pmI2CWriteByte(data[i]))
			return false;
	
  // Send STOP condition
	pmI2CStop();
	
	return true;
}