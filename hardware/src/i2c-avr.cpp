#include <util/delay.h>
#include <util/twi.h>

#include "avr-new.h"
#include "i2c.h"

/*
Refer to Atmega328p datasheet, section 21:
https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
*/

#define I2C_PORT PORTC
#define I2C_DDR DDRC
#define I2C_SCL_PIN 5
#define I2C_SDA_PIN 4

class i2c_bus_controller::i2c_bus_impl {
public:
  i2c_bus_impl(const uint32_t &scl_frequency_hz) {
    if (!set_scl_freq(scl_frequency_hz))
      return;

    // Pull I2C pins high and leave them to the integrated I2C module
    I2C_PORT |= _BV(I2C_SDA_PIN) | _BV(I2C_SCL_PIN);
    I2C_DDR &= ~(_BV(I2C_SDA_PIN) | _BV(I2C_SCL_PIN));

    // Enable I2C(TWI)
    TWCR |= _BV(TWEN);
  }

  ~i2c_bus_impl() { TWCR = 0; }

  bool set_scl_freq(const uint32_t &scl_frequency_hz) {
    /*
    Solving the formula provided by Atmel in Unit 21.5.2 of datasheet for TWBR:
    SCL frequency = CPU frequency / (16 + 2 * TWBR * Prescaler value)
    we'll get:
    */
    uint32_t twbr = (static_cast<uint32_t>(F_CPU) / scl_frequency_hz - 16) / 2;
    uint8_t prescaler = 0;

    // Use prescaler if necessary
    while (twbr > UCHAR_MAX) {
      /*
      AtMega328p offers 4 possible prescaler values: 1, 4, 16, 64 by setting
      TWPS0 and TWPS1 bits in TWSR register. 0 corresponds to prescaler value of
      1; 3 corresponds to prescaler value of 64. If twbr value does not fit into
      8-bit register, divide twbr by 4 while increasing prescaler option.
      */
      twbr <<= 2;
      ++prescaler;
      // maximum prescaling reached, abort construction.
      if (prescaler > 3)
        return false;
    }

    // Configure data rate in hardware
    TWBR = static_cast<uint8_t>(twbr);

    // Set prescaler bits
    TWSR |= (_BV(TWPS0) | _BV(TWPS1)) & prescaler;

    return true;
  }

  bool send_start() {
    // Send START condition
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);

    // Wait for TWINT flag to set
    while (!(TWCR & (1 << TWINT)))
      ;

    // Check error
    if (TW_STATUS != TW_START && TW_STATUS != TW_REP_START)
      return false;

    return true;
  }

  void send_stop() { TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO); }

  bool set_address(uint8_t address, bool write) {
    TWDR = (address << 1) | (uint8_t)(write ? 0 : 1);
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag to set
    while (!(TWCR & (1 << TWINT)))
      ;

    if (!(TW_STATUS == TW_MT_SLA_ACK || TW_STATUS == TW_MR_SLA_ACK))
      return false;

    return true;
  }

  bool write_byte(const uint8_t byte) {
    // Transmit 1 byte
    TWDR = byte;
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag to set
    while (!(TWCR & (1 << TWINT)))
      ;

    // Wait for ACK
    if (TW_STATUS != TW_MT_DATA_ACK)
      return false;

    return true;
  }

  bool read_byte(uint8_t &byte, bool sendAck) {
    TWCR = (1 << TWINT) | (1 << TWEN) | (sendAck ? (1 << TWEA) : 0);

    // Wait for operation to complete
    while (!(TWCR & (1 << TWINT)))
      ;

    if (sendAck) {
      if (TW_STATUS != TW_MR_DATA_ACK)
        return false;
    } else {
      if (TW_STATUS != TW_MR_DATA_NACK)
        return false;
    }

    byte = TWDR;
    return true;
  }
};

i2c_bus_controller::i2c_bus_controller(const uint32_t &scl_frequency_hz)
    : m_impl(new i2c_bus_impl(scl_frequency_hz)) {}

bool i2c_bus_controller::read(const uint8_t address, const ibytevect &reg_addr,
                              ibytevect &result) {
  uint8_t length = result.size();
  if (reg_addr.empty())
    return false;

  // Send START condition
  if (!m_impl->send_start())
    return false;

  //  Set I2C bus address
  if (!m_impl->set_address(address, true))
    return false;

  //  Set the first register to read from
  for (uint8_t i = 0; i < reg_addr.size(); ++i)
    if (!m_impl->write_byte(reg_addr[i]))
      return false;

  // Send repeated START
  if (!m_impl->send_start())
    return false;

  if (!m_impl->set_address(address, false))
    return false;

  //  Read multiple data bytes and send ack after each one
  for (uint8_t i = 0; i < length - 1; ++i)
    if (!m_impl->read_byte(result[i], true))
      return false;

  //  Read single final data byte and do not send ack
  if (!m_impl->read_byte(result[length - 1], false))
    return false;

  // Send STOP condition
  m_impl->send_stop();

  return true;
}

bool i2c_bus_controller::write(const uint8_t address, const ibytevect &reg_addr,
                               const ibytevect &data) {
  uint8_t length = data.size();
  if (reg_addr.empty())
    return false;

  // Send START condition
  if (!m_impl->send_start())
    return false;

  // Send address with WRITE flag
  if (!m_impl->set_address(address, true))
    return false;

  // Set the first register to write to
  for (uint8_t i = 0; i < reg_addr.size(); ++i)
    if (!m_impl->write_byte(reg_addr[i]))
      return false;

  // Send bytes
  for (uint8_t i = 0; i < length; ++i)
    if (!m_impl->write_byte(data[i]))
      return false;

  // Send STOP condition
  m_impl->send_stop();

  return true;
}
