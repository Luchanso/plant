#include <avr/interrupt.h>

#include "timers.h"

/*
Refer to Atmega328p datasheet, section 15 and 17
https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
*/

/*
Together with prescaler value of 256, Timer 1 compare match A interrupt will
be triggered once a second, i.e. F_CPU / 256 / 62500 = 1;
where F_CPU usually equals to 16MHz for 5v Arduino Nano board with Atmega328p.
*/
#define OCR1A_CYCLES_FOR_ONE_SECOND 62500

/*
Commonly used standard in ModBus RTU for "line silence" is 3.5 character long,
but no less than 1.75 ms to determine message end. Let's use that here as well.

In case of 9600 baud 8N1, i.e. 1 start bit, 8 bits of data, No parity, 1 stop bit,
one character consists of 10 bits. With the speed of 9600 bits per second,
one character will be sent in (10 / 9600) seconds,
which rougly equals to 0,00104 seconds, or 1,04 milliseconds.
This means, that the 3.5 character line silence will take 3.64 ms 
3.64 milliseconds / 16 microseconds = 227.5
But as only integer value is allowed, rounding up to 228 will do the trick.
*/
#define OCR1B_CYCLES_FOR_9600_BAUD_IDLE 228

// Function pointers
void (*oneSecondCallback)() = 0;
void (*lineIdleCallback)() = 0;

void pmSetupTimersInterrupts(void (*callbackA)(), void (*callbackB)()) {
  cli();  //stop interrupts

  TCCR1A = 0;  // Clear TCCR1A register
  TCCR1B = 0;  // Clear TCCR1B register
  TCNT1 = 0;   // Reset Timer CouNTer 1

  /*
  Adjust Timer 1 Control register B to set the prescaler to F_CPU/256.
  With F_CPU == 16MHz, this means, that every 16 microseconds value of TCNT1
  will be incremented by 1.
  */
  TCCR1B |= (1 << CS12);

  oneSecondCallback = callbackA;
  lineIdleCallback = callbackB;

  sei();  //allow interrupts
}

void pmStartOneSecondTimer() {
  /*
  The "Output Compare Register (Timer) 1 A" is used to trigger interrupt
  whenever OCR1A == TCNT1.
  */
  OCR1A = TCNT1 + OCR1A_CYCLES_FOR_ONE_SECOND;

  // Enable timer 1 output compare A match interrupt.
  TIMSK1 |= (1 << OCIE1A);
}

void pmStopOneSecondTimer() {
  // Disable timer 1 output compare A match interrupt.
  TIMSK1 &= ~(1 << OCIE1A);
}

void pmStartSerialLineIdleTimer() {
  // Enable timer 1 output compare B match interrupt
  OCR1B = TCNT1 + OCR1B_CYCLES_FOR_9600_BAUD_IDLE;
  TIMSK1 |= (1 << OCIE1B);
}

void pmStopSerialLineIdleTimer() {
  // Disable timer 1 output compare B match interrupt
  TIMSK1 &= ~(1 << OCIE1B);
}

ISR(TIMER1_COMPA_vect) {
  // Advance timer interrupt one second ahead.
  OCR1A += OCR1A_CYCLES_FOR_ONE_SECOND;
  (*oneSecondCallback)();
}

ISR(TIMER1_COMPB_vect) {
  // Disable this interrupt
  TIMSK1 &= ~(1 << OCIE1B);
  (*lineIdleCallback)();
}