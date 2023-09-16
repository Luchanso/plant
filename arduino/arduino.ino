#include "proto.h";

#define MAX_BUFFER_SIZE 32

plantMessage *inPm;
plantMessage *outPm;

uint8_t bufferSize = MAX_BUFFER_SIZE;
uint8_t buffer[MAX_BUFFER_SIZE] = {};
uint8_t lastResult = 0;
_Bool LEDState = 1;

void setup() {
  inPm = pmCreate();
  outPm = pmCreate();

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, LEDState);
  delay(1000);
  LEDState = !LEDState;

  lastResult = analogRead(A0);

  pmFillADCResult(lastResult, outPm);
  if( pmSerialize(outPm, buffer, &bufferSize) )
    Serial.write(buffer, bufferSize);
}