#include "proto.h"
#include "timers.h"
#include "usart.h"

plantMessage *inPm = NULL;
plantMessage *outPm = NULL;

uint8_t lastResult = 0;
bool LEDState = true;

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

  default:
    pmFillBadRequest(outPm);
  }

  pmUSARTSend(outPm);
}

// 3a 01 00 70
void loop() {
  if (hasData) {
    // USARTSendDebugText("hasData = true\n");
    uint8_t *buffer = NULL;
    uint8_t bytes = pmUSARTCopyReceivedData(&buffer);

    // USARTSendDebugText("loop() bytes received: ");
    // USARTSendDebugNumber(bytes);
    // USARTSendDebugText("\n");

    // for (uint8_t i = 0; i < bytes; i++) {
    //   USARTSendDebugNumber(buffer[i]);
    //   USARTSendDebugText(" ");
    // }

    // USARTSendDebugText("\n");

    if (bytes) {
      pmParseResult parseResult = prUndefined;

      do {
        parseResult = pmParse(buffer, bytes, inPm);

        // USARTSendDebugText("parseResult = ");
        // USARTSendDebugNumber(parseResult);
        // USARTSendDebugText("\n");

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

    // USARTSendDebugText("loop() end\n");
  }
  // todo: sleep mode
}