#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Various functions to convert integers into and from different stuff

// save diagnostic state
#pragma GCC diagnostic push

// turn off the specific warning. Can also use "-Wall"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include <etl/endianness.h>

// turn the warnings back on
#pragma GCC diagnostic pop

#include <etl/type_traits.h>
#include <etl/vector.h>
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

template <auto T> using bytevect = etl::vector<uint8_t, T>;
using ibytevect = etl::ivector<uint8_t>;
#define alloc_bytevect(X, Y) bytevect<Y> X(Y)

/**
 * Get a value from a byte vector, assuming value is little-endian; move
 * iterator forward by a size of I.
 * @return converted value to a native(little) endianness
 */
template <typename I> void get_le(I &val, uint8_t *&iterator) {
  static_assert(etl::is_pod<I>::value,
                "This function is applicable only for Plain Old Data types");

  memcpy(&val, iterator, sizeof(I));

  if (etl::endian::native == etl::endian::big)
    val = etl::reverse_bytes(val);

  iterator += sizeof(I);
}

/**
 * Put a value into a byte buffer, assuming value is little-endian; move
 * iterator forward by a size of I.
 * @param val value to set
 */
template <typename I> void set_le(I val, uint8_t *&iterator) {
  static_assert(etl::is_pod<I>::value,
                "This function is applicable only for Plain Old Data types");

  if (etl::endian::native == etl::endian::big)
    val = etl::reverse_bytes(val);

  memcpy(iterator, &val, sizeof(I));
  iterator += sizeof(I);
}

/**
 * Get a value from a byte buffer, assuming it is big-endian; move iterator
 * forward by a size of I.
 * @return converted value to a native(little) endianness
 */
template <typename I> void get_be(I &val, uint8_t *&iterator) {
  static_assert(etl::is_pod<I>::value,
                "This function is applicable only for Plain Old Data types");

  memcpy(&val, iterator, sizeof(I));

  if (etl::endian::native == etl::endian::little)
    val = etl::reverse_bytes(val);

  iterator += sizeof(I);
}

/**
 * Put a value into a byte buffer, assuming value is big endian; move iterator
 * forward by a size of I.
 * @param val value to set
 */
template <typename I> void set_be(I val, uint8_t *&iterator) {
  static_assert(etl::is_pod<I>::value,
                "This function is applicable only for Plain Old Data types");

  if (etl::endian::native == etl::endian::little)
    val = etl::reverse_bytes(val);

  memcpy(iterator, &val, sizeof(I));
  iterator += sizeof(I);
}
