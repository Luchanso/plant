#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Various functions to convert integers into and from different stuff

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

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
 * Fetches an (un)signed short integer from @p rawBuffer into @p destination,
 * assuming that both rawBuffer and destination is an unsigned or signed short
 * (16 bits/2 bytes) little-endian(avr-gcc default) integer.
 * Will increment @p rawBuffer by 2.
 * @param rawBuffer pointer to a buffer containing raw bytes, will be incremented
 * @param destination pointer to an unsigned or signed short integer
 */
void getInt16FromLEBuffer(uint8_t **rawBuffer, void *destination);

/**
 * Fetches an (un)signed char from @p rawBuffer into @p destination, assuming
 * that destination is an unsigned or signed char (8 bits/1 byte).
 * Will increment @p rawBuffer by 1.
 * @param rawBuffer pointer to a buffer containing raw bytes, will be incremented
 * @param destination pointer to an unsigned or signed short integer
 */
void getInt8FromBuffer(uint8_t **rawBuffer, void *destination);

#if defined(__cplusplus)
}
#endif