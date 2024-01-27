#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

/*
Basic I2C (also known as TWI in Atmel manuals) abstraction layer for
AtMega328p/PlantMonitor. This code assumes there is only one primary device on
the bus and it's this one.
*/

#include "convert_util.h"
#include <etl/memory.h>
#include <etl/type_traits.h>
#include <etl/vector.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Class to control built-in I2C/TWI bus controller as primary device on the
 * bus.
 */
class i2c_bus_controller {
  // Hardware-specific implementation is hidden.
  class i2c_bus_impl;
  etl::unique_ptr<i2c_bus_impl> m_impl;

  // It's not possible to just copy or move the hardware around.
  i2c_bus_controller(const i2c_bus_controller &) = delete;
  i2c_bus_controller(i2c_bus_controller &&other) = delete;
  i2c_bus_controller &operator=(const i2c_bus_controller &) = delete;
  i2c_bus_controller &operator=(i2c_bus_controller &&other) = delete;

  /**
   * Read data from I2C bus. Will do burst read if length the capacity of a @p
   * result is greater than 1.
   * @param[in] address device address.
   * @param[in] reg_addr request to send to a peripheral before reading data.
   * @param[out] result buffer to read data to. If size of a @p
   * result is greater than 1, function will read multiple registers in a row.
   * @return true if operation was successful.
   */
  bool read(const uint8_t addr, const ibytevect &reg_addr, ibytevect &result);

  /**
   * Write data to I2C bus. Will do burst write if @p length is greater than 1.
   * @param address device address.
   * @param reg_addr request to send to a peripheral before writing data.
   * If length is greater than 1, function will write multiple registers in a
   * row.
   * @param data buffer to read data from.
   * @return true if operation was successful.
   */
  bool write(const uint8_t addr, const ibytevect &reg, const ibytevect &data);

  friend class i2c_peripheral;

public:
  i2c_bus_controller(const uint32_t &scl_frequency_hz = 400000);
  ~i2c_bus_controller() = default;

  /**
   * Check I2C readiness status.
   * @return true if controller is initalized and isn't busy sending or
   * receiving any data.
   */
  bool ready();
};

// Base class for implementing secondary devices on the bus
class i2c_peripheral {
  uint8_t m_bus_address = 0;
  i2c_bus_controller *m_bus_controller = nullptr;

protected:
  i2c_peripheral(uint8_t bus_address, i2c_bus_controller *bus_controller)
      : m_bus_address(bus_address), m_bus_controller(bus_controller) {}

  template <typename T> bool write(const T &reg_addr, const ibytevect &data) {
    static_assert(etl::is_unsigned<T>::value,
                  "reg_addr can only have unsigned integer type");

    alloc_bytevect(addr, sizeof(T));
    for (uint8_t i = 0; i < sizeof(T); ++i)
      addr[i] = (reg_addr >> (8 * i)) & 0xFF;

    return m_bus_controller->write(m_bus_address, addr, data);
  }

  template <typename T>
  bool read(const T &reg_addr, etl::ivector<uint8_t> &data) {
    static_assert(etl::is_unsigned<T>::value,
                  "reg_addr can only have unsigned integer type");

    alloc_bytevect(addr, sizeof(T));
    for (uint8_t i = 0; i < sizeof(T); ++i)
      addr[i] = (reg_addr >> (8 * i)) & 0xFF;

    return m_bus_controller->read(m_bus_address, addr, data);
  }
};
