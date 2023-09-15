/// By gh/BortEngineerDude for gh/Luchanso

#pragma once

#include <inttypes.h>

/*
 Simple binary protocol.

 Example:
 ┌───────────────── Message start marker, always 0x3A
 │  ┌────────────── Message code: 0x02 - measurement result
 │  │  ┌─────────── Payload length: 0x01 - payload contains one byte
 │  │  │  ┌──────── Message payload: 0xA0 - ADC measurement = 160
 3A 02 01 A0 77 ─── Maxim/Dallas integrated iButton CRC8
Hence, the smallest possible message using this protocol is 4 bytes long.
*/

/// Plant message code, huh.
typedef enum {
  pmcUndefined = 0,
  pmcMeasurementRequest = 1,
  pmcMeasurementResult = 2,
  pmcBadCRC = 3
} plantMessageCode;

typedef struct plantMessage plantMessage;

typedef enum {
  prUndefined = 0, // no attempt to parse message was made
  prOk,            // message succesfully fetched
  prBadCrc,        // message failed to pass CRC
  prIncomplete     // message is yet to be received completely
} pmcParseResult;

/**
 * Create a plant message.
 * @return zero-initialized plantMessage.
 */
plantMessage* pmCreate();

/**
 * Fill a \p result with a ADC result message.
 * @param[in] ADCValue Value to set
 * @param[out] result message to fill
 * @return true on success.
 */
_Bool pmFillADCResult(const uint8_t ADCValue, struct plantMessage* result);

/**
 * Fill a \p result with a bad CRC message.
 * @param[out] result message to fill
 * @return true on success.
 */
_Bool pmFillBadCRC(plantMessage* result);

/**
 * Get a plant message code
 * @param msg message
 * @return plant message code
 */
plantMessageCode pmGetMessageCode(const plantMessage* msg);

/**
 * Delete plant message in a proper manner.
 * @param[in,out] msg message to destroy. Will be NULL after running the function.
 */
void pmDestroy(plantMessage* msg);

/**
 * Create raw byte array from a plantMessage
 * @param[in] input message to create array from
 * @param[out] buffer buffer to write message to
 * @param[in,out] bufferSize on input - max buffer size, on output - total output raw byte array length
 * @return true on success
 */
_Bool pmSerialize(const plantMessage* input, uint8_t* buffer, uint8_t* bufferSize);

/**
 * Attempt to parse a message
 * @param[in]  buffer raw received data buffer 
 * @param[in]  bufferSize buffer data size
 * @param[out] result parsed message
 * @return enum pmcParseResult describing state of message stored by result pointer.
 * NOTE: On succesful result or bad CRC raw message will be overwritten by 0's in \p buffer,
 *       if message is incomplete \p buffer will remain untouched.
 */
pmcParseResult pmParse(uint8_t* buffer, uint8_t bufferSize, plantMessage* result);