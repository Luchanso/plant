#include "proto.h"
#include "timers.h"
#include "usart.h"
#include "i2c.h"
#include "ds3231.h"

plantMessage *inPm = NULL;
plantMessage *outPm = NULL;

uint8_t lastResult = 0;
bool LEDState = true;
time systemTime = {};

// Note: variables modified by Interrupt callbacks must have volatile keyword.
volatile bool hasData = false;

// Try to keep callbacks short, use the main loop to do the heavy lifting.
void serialLineIdle() { hasData = true; }

void oneSecond() {
  digitalWrite(LED_BUILTIN, LEDState);
  LEDState = !LEDState;
}

void setup() {
  pmSetupTimersInterrupts(&oneSecond, &serialLineIdle);
  pmStartOneSecondTimer();
  pmUSARTInit();
  pmI2CInit();

  inPm = pmCreate();
  outPm = pmCreate();

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

void handleIncomingMessage(const plantMessage *pm) {
  plantMessageCode code = pmGetMessageCode(pm);

  switch (code) {
  case pmcMeasurementRequest:
    lastResult = analogRead(A0);
    pmFillADCResult(lastResult, outPm);
    break;

  case pmcGetRTCTime:
    if(pmDS3231ReadTime(&systemTime))
      pmFillTime(&systemTime, outPm);
    else
      pmFillHardwareError(outPm);
    break;

  case pmcSetRTCTime:
    if(pmGetTime(inPm, &systemTime)) {
      if(pmDS3231SetTime(&systemTime))
        pmFillTime(&systemTime, outPm);
      else
        pmFillHardwareError(outPm);
    }
    break;

  default:
    pmFillBadRequest(outPm);
  }

  pmUSARTSend(outPm);
}

// 3a 01 00 70 -- measurement
// 3a 03 00 e1 -- time
void loop() {
  if (hasData) {
    uint8_t *buffer = NULL;
    uint8_t bytes = pmUSARTCopyReceivedData(&buffer);

    if (bytes) {
      pmParseResult parseResult = prUndefined;

      do {
        parseResult = pmParse(buffer, bytes, inPm);

        switch (parseResult) {
        case prOk:
          handleIncomingMessage(inPm);
          break;

        case prBadCrc:
          pmFillBadCRC(outPm);
          pmUSARTSend(outPm);
          break;

        case prIncomplete: // todo: improve
          if (bytes == USART_BUFFER_SIZE)
            pmUSARTClearRxBuffer();
          break;

        case prUndefined:
          pmUSARTClearRxBuffer();
          break;
        }
      } while (parseResult != prUndefined && parseResult != prIncomplete);

      if (buffer) {
        free(buffer);
        buffer = NULL;
      }
    }
    hasData = false;
  }
}