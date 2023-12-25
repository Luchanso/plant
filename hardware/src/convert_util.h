#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Various functions to convert integers into and from different stuff

#include <etl/endianness.h>
#include <etl/type_traits.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Convert binary-coded decimal into native decimal number.
 * @param value byte containing binary-coded decimal
 * @return native decimal value
 */
uint8_t convertFromBCD(uint8_t value);

/**
 * Convert native decimal number into binary-coded decimal.
 * @param value byte containing native decimal number
 * @return binary-coded decimal
 */
uint8_t convertToBCD(uint8_t value);

/**
 * Get a value from a byte buffer, assuming it is little endian as well as host;
 * move iterator forward by a size of T.
 * @param val reference to any value of POD type
 * @param iterator buffer iterator containing raw bytes; will be
 * incremented by a size of T
 * @return converted value to a native(little) endianness
 */
template <typename T> void get_le(T &val, uint8_t *&iterator) {
  static_assert(etl::is_pod<T>::value,
                "This function is applicable only for Plain Old Data types");
  memcpy(&val, iterator, sizeof(T));
  iterator += sizeof(T);
}

/**
 * Put a value into a byte buffer, assuming value and vector itself are
 * little-endian.
 * @param
 */
template <typename T> void set_le(const T &val, uint8_t *&iterator) {
  static_assert(etl::is_pod<T>::value,
                "This function is applicable only for Plain Old Data types");
  memcpy(iterator, &val, sizeof(T));
  iterator += sizeof(T);
}

/**
 * Get a value from a byte buffer, assuming it is big endian; move iterator
 * forward by a size of T.
 * @param val reference to any value of POD type
 * @param iterator buffer iterator containing raw bytes; will be
 * incremented by a size of T
 * @return converted value to a native(little) endianness
 */
template <typename T> void get_be(T &val, uint8_t *&iterator) {
  static_assert(etl::is_pod<T>::value,
                "This function is applicable only for Plain Old Data types");
  memcpy(&val, iterator, sizeof(T));
  val = etl::ntoh(val);
  iterator += sizeof(T);
}
