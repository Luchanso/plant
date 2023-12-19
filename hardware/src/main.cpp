#include <avr/io.h>
#include <stdlib.h>

#include <etl/circular_buffer.h>

#include "bme280.h"
#include "ds3231.h"
#include "i2c.h"
#include "proto.h"
#include "timers.h"
#include "usart.h"

#define LED_PORT PORTB
#define LED_PIN 5
#define LED_DDR DDRB

plantMessage *inPm = NULL;
plantMessage *outPm = NULL;

uint8_t lastResult = 0;
volatile bool LEDState = true;
volatile bool debug = false;

time systemTime = {};
const char *weekdays[] = {"Monday", "Tuesday",  "Wednesday", "Thursday",
                          "Friday", "Saturday", "Sunday"};

BME280Data *BME280 = NULL;

// Note: variables modified by Interrupt callbacks must have volatile keyword.
volatile bool hasData = false;

// Try to keep callbacks short, use the main loop to do the heavy lifting.
void serialLineIdle() { hasData = true; }

void oneSecond() {
  //  digitalWrite(LED_BUILTIN, LEDState);

  if (LEDState)
    LED_PORT |= _BV(LED_PIN);
  else
    LED_PORT &= ~_BV(LED_PIN);

  LEDState = !LEDState;

  debug = true;
}

void setup() {
  pmSetupTimersInterrupts(&oneSecond, &serialLineIdle);
  pmStartOneSecondTimer();
  pmUSARTInit();
  pmI2CInit();

  BME280 = createBME280();

  inPm = pmCreate();
  outPm = pmCreate();

  // initialize digital pin LED_BUILTIN as an output.
  // pinMode(LED_BUILTIN, OUTPUT);
  LED_DDR |= _BV(LED_PIN);
}

void handleIncomingMessage(const plantMessage *pm) {
  plantMessageCode code = pmGetMessageCode(pm);

  switch (code) {
  case pmcMeasurementRequest:
    // lastResult = analogRead(A0);
    pmFillADCResult(lastResult, outPm);
    break;

  case pmcGetRTCTime:
    if (DS3231ReadTime(&systemTime))
      pmFillTime(&systemTime, outPm);
    else
      pmFillHardwareError(outPm);
    break;

  case pmcSetRTCTime:
    if (pmGetTime(inPm, &systemTime)) {
      if (DS3231SetTime(&systemTime))
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
int main() {
  setup();
  etl::circular_buffer<uint8_t, 8> test;
  // test.push(1);
  // test.push(2);

  while (1) {

    if (debug) {
      debug = false;

      if (DS3231IsAvailable()) {
        pmUSARTSendDebugText("DS3231 is available\r\n");
        if (DS3231ReadTime(&systemTime)) {
          pmUSARTSendDebugNumber(systemTime.year);
          pmUSARTSendDebugText(".");
          pmUSARTSendDebugNumber(systemTime.month);
          pmUSARTSendDebugText(".");
          pmUSARTSendDebugNumber(systemTime.dayOfMonth);
          pmUSARTSendDebugText(" ");

          pmUSARTSendDebugNumber(systemTime.hours);
          pmUSARTSendDebugText(":");
          pmUSARTSendDebugNumber(systemTime.minutes);
          pmUSARTSendDebugText(":");
          pmUSARTSendDebugNumber(systemTime.seconds);
          pmUSARTSendDebugText(", ");
          pmUSARTSendDebugText(weekdays[systemTime.dayOfWeek - 1]);
          pmUSARTSendDebugText("\r\n\r\n");
        } else {
          pmUSARTSendDebugText("DS3231 failed to read time\r\n");
        }
      } else {
        pmUSARTSendDebugText("DS3231 is unavailable\r\n");
      }

      if (BME280IsAvailable()) {
        pmUSARTSendDebugText("BME280 is available\r\n");
        if (BME280IsIdle()) {
          pmUSARTSendDebugText("BME280 is idle\r\n");

          BME280GetData(BME280);
          if (BME280->pressure) {
            pmUSARTSendDebugText("Temperature = ");
            pmUSARTSendDebugNumber(BME280->temperature);
            pmUSARTSendDebugText(" / 100 C\r\n");

            pmUSARTSendDebugText("Pressure = ");
            pmUSARTSendDebugNumber(BME280->pressure);
            pmUSARTSendDebugText(" Pa\r\n");

            pmUSARTSendDebugText("Humidity = ");
            pmUSARTSendDebugNumber(BME280->humidity / 1024);
            pmUSARTSendDebugText(" %\r\n\r\n");
          }

          if (BME280StartMeasurement())
            pmUSARTSendDebugText("BME280 has started measurement\r\n");

        } else {
          pmUSARTSendDebugText("BME280 is measuring...\r\n");
        }
      } else {
        pmUSARTSendDebugText("BME280 is unavailable\r\n");
      }
    }

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
}
