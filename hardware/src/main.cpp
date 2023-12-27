#include <stdlib.h>

#include "avr-gpio.h"
#include "bme280.h"
#include "ds3231.h"
#include "i2c.h"
#include "proto.h"
#include "scd40.h"
#include "timers.h"
#include "usart.h"

#define LED_PORT B
#define LED_PIN 5

plantMessage *inPm = NULL;
plantMessage *outPm = NULL;

uint8_t lastResult = 0;
volatile bool LEDState = true;
volatile bool debug = false;

i2c_bus_controller i2c;
bme_280 bme280(&i2c);
ds_3231 ds3231(&i2c);
scd_40 scd40(&i2c);

time systemTime = {};
const char *weekdays[] = {"Monday", "Tuesday",  "Wednesday", "Thursday",
                          "Friday", "Saturday", "Sunday"};

// Note: variables modified by Interrupt callbacks must have volatile keyword.
volatile bool hasData = false;

// Try to keep callbacks short, use the main loop to do the heavy lifting.
void serialLineIdle() { hasData = true; }

void oneSecond() {
  set_pin(LED_PORT, LED_PIN, LEDState);
  LEDState = !LEDState;

  debug = true;
}

void setup() {
  pmSetupTimersInterrupts(&oneSecond, &serialLineIdle);
  pmStartOneSecondTimer();
  pmUSARTInit();

  inPm = pmCreate();
  outPm = pmCreate();

  if (!scd40.start_measurement())
    pmUSARTSendDebugText("SCD40 failed to start measurement\r\n");

  // initialize digital pin LED_BUILTIN as an output.
  set_output_pin(LED_PORT, LED_PIN);
}

void handleIncomingMessage(const plantMessage *pm) {
  plantMessageCode code = pmGetMessageCode(pm);

  switch (code) {
  case pmcMeasurementRequest:
    // lastResult = analogRead(A0);
    pmFillADCResult(lastResult, outPm);
    break;

  case pmcGetRTCTime:
    if (ds3231.get_time(systemTime))
      pmFillTime(&systemTime, outPm);
    else
      pmFillHardwareError(outPm);
    break;

  case pmcSetRTCTime:
    if (pmGetTime(inPm, &systemTime)) {
      if (ds3231.set_time(systemTime))
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

  while (1) {

    if (debug) {
      debug = false;

      if (scd40.measurement_ready()) {
        scd_40::measurement_data data;

        if (ds3231.available()) {
          if (ds3231.get_time(systemTime)) {
            pmUSARTSendDebugText("\r\n");
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

            uint16_t temperature = 0;
            if (ds3231.get_temperature(temperature)) {
              pmUSARTSendDebugText("\r\n DS3231 Temperature = ");
              pmUSARTSendDebugNumber(temperature);
              pmUSARTSendDebugText(" / 100 C \r\n");
            } else {
              pmUSARTSendDebugText("\r\n DS3231 failed to read temperature.");
            }
          } else {
            pmUSARTSendDebugText("DS3231 failed to read time\r\n");
          }
        } else {
          pmUSARTSendDebugText("DS3231 is unavailable\r\n");
        }

        if (scd40.get_data(data)) {
          pmUSARTSendDebugText("SCD40 data\r\n");
          pmUSARTSendDebugText(" CO2 ppm = ");
          pmUSARTSendDebugNumber(data.co2ppm);
          pmUSARTSendDebugText("\r\n Temperature = ");
          pmUSARTSendDebugNumber(data.temperature);
          pmUSARTSendDebugText(" / 100 C \r\n Humidity = ");
          pmUSARTSendDebugNumber(data.humidity);
          pmUSARTSendDebugText(" % \r\n");
        } else {
          pmUSARTSendDebugText("Failed to get SCD40 data\r\n");
        }

        if (bme280.available()) {
          if (bme280.idle()) {
            bme_280::measurement_data data;

            if (bme280.get_data(data)) {
              pmUSARTSendDebugText("BME280 data\r\n");
              pmUSARTSendDebugText(" Temperature = ");
              pmUSARTSendDebugNumber(data.temperature);

              pmUSARTSendDebugText(" / 100 C\r\n Pressure = ");
              pmUSARTSendDebugNumber(data.pressure);

              pmUSARTSendDebugText(" / 10 hPa\r\n Humidity = ");
              pmUSARTSendDebugNumber(data.humidity);
              pmUSARTSendDebugText(" %\r\n\r\n");

              if (!scd40.set_compensation_pressure(data.pressure / 10))
                pmUSARTSendDebugText(
                    "Failed to set SCD40 compensation pressure\r\n");
            } else
              pmUSARTSendDebugText("BME280 failed to get data\r\n");

            if (!bme280.start_measurement())
              pmUSARTSendDebugText("BME280 failed to start measurement\r\n");
          }
        } else {
          pmUSARTSendDebugText("BME280 is unavailable\r\n");
        }
      } else {
        pmUSARTSendDebugText(".");
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
