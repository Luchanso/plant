/// By gh/BortEngineerDude for gh/Luchanso

#include "proto.h"
#include <util/crc16.h>
#include <string.h>
#include <stdlib.h>

#define PMC_MIN_MSG_LENGTH 4
#define PMC_MSG_START_BYTE 0x3A
#define PMC_MSG_PAYLOAD_OFFSET 3

struct plantMessage{
  plantMessageCode  code;
  uint8_t           payloadSize;
  uint8_t*          payload;
};

/**
 * Calculate CRC.
 * @param buffer raw bytes to calculate CRC for
 * @param length buffer size
 * @return For buffer with a correct CRC appended at the very end
 *         returns 0 if the CRC passed, any non-zero value means the message is damaged.
 *         For buffer without a CRC in the end returns the CRC.
 */
static uint8_t CRC(const uint8_t* buffer, const uint8_t length) {
	uint8_t crc = 0;

	for (uint8_t i = 0; i < length; i++)
	    crc = _crc_ibutton_update(crc, buffer[i]);

	return crc;
}

/**
 * Adjust payload ptr and payload size inside plantMessage to accomodate required size of payload.
 * @param msg[in,out] plantMessage where adjustment must be made
 * @param requiredPayloadSize size of payload that will be written to plantMessage
 */
static void adjustPayloadSize(plantMessage* msg, uint8_t requiredPayloadSize) {
  if(msg->payload) {
    if(requiredPayloadSize == 0) {
      free(msg->payload);
      msg->payload = NULL;
    } else if(msg->payloadSize != requiredPayloadSize) {
      realloc(msg->payload, requiredPayloadSize);
    }
  } else if(requiredPayloadSize) {
    msg->payload = malloc(requiredPayloadSize);
  }
  msg->payloadSize = requiredPayloadSize;
}

plantMessage* pmCreate() {
  return calloc(1, sizeof(plantMessage));
}

void pmDestroy(plantMessage* msg) {
  if(!msg)
    return;

  if(msg->payload)
    free(msg->payload);
  
  free(msg);

  msg = NULL;
}

_Bool pmFillADCResult(const uint8_t ADCValue, plantMessage* result) {
  adjustPayloadSize(result, 1);
  result->code = pmcMeasurementResult;
  *result->payload = ADCValue;

  return 1;
}

_Bool pmFillBadCRC(plantMessage* result) {
  adjustPayloadSize(result, 0);
  result->code = pmcBadCRC;

  return 1;
}

pmcParseResult pmParse(uint8_t* buffer, uint8_t bufferSize, plantMessage* result) {
  if(!buffer || !result)
    return prUndefined;

  if(bufferSize < PMC_MIN_MSG_LENGTH)
    return prIncomplete;

  uint8_t* iterator = buffer;
  uint8_t* end = iterator + bufferSize;

  // Find message start
  while(iterator != end)
    if(*iterator == PMC_MSG_START_BYTE)
      break;

  bufferSize = end - iterator;
  uint8_t payloadLength = iterator[2]; 
  uint8_t msgSize = PMC_MIN_MSG_LENGTH + payloadLength;

  if(bufferSize < msgSize)
    return prIncomplete;

  if(CRC(iterator, msgSize)) {
    memset(iterator, 0, msgSize);
    return prBadCrc;
  }

  adjustPayloadSize(result, payloadLength);

  if(payloadLength)
    memcpy(result->payload, iterator + PMC_MSG_PAYLOAD_OFFSET, payloadLength);

  result->code = iterator[1];
  memset(iterator, 0, msgSize);

  return prOk;
}

_Bool pmSerialize(const plantMessage* input, uint8_t* buffer, uint8_t* bufferSize) {
  uint8_t msgSize = PMC_MIN_MSG_LENGTH + input->payloadSize;
  if(*bufferSize < msgSize)
    return 0;

  buffer[0] = PMC_MSG_START_BYTE;
  buffer[1] = input->code;
  buffer[2] = input->payloadSize;

  if(input->payloadSize)
    memcpy(buffer + PMC_MSG_PAYLOAD_OFFSET, input->payload, input->payloadSize);

  buffer[msgSize - 1] = CRC(buffer, msgSize - 1);

  return 1;
}