#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

/* Simple pin control for Arduino Nano / AtMega328p

Arduino pins to AtMega328p pin mapping:

PORTD:
D0  - PD0  PCINT16  RX0
D1  - PD1  PCINT17  TX1
D2  - PD2  PCINT18  EXTINT0
D3  - PD3  PCINT19  EXTINT1  PWM-OC2B
D4  - PD4  PCINT20  T0       XCK
D5  - PD5  PCINT21  T1       PWM-OC0B
D6  - PD6  PCINT22  AIN0     PWM-OC0A
D7  - PD7  PCINT23  AIN1

PORTB:
D8  - PB0  PCINT0   CLK0     ICP1
D9  - PB1  PCINT1   PWM-OC1B
D10 - PB2  PCINT2   !SS      PWM-OC1B
D11 - PB3  PCINT3   MOSI     PWM-OC2
D12 - PB4  PCINT4   MISO
D13 - PB5  PCINT5   SCK      LED_BUILTIN

PORTC:
A0  - PC0  PCINT8    ADC0
A1  - PC1  PCINT9    ADC1
A2  - PC2  PCINT10   ADC2
A3  - PC3  PCINT11   ADC3
A4  - PC4  PCINT12   I2C-SDA ADC4
A5  - PC5  PCINT13   I2C-SCL ADC5
RST -!PC6  RESET     DO NOT OVERRIDE!

Dedicated ADC-only pins:
A6  - ADC6
A7  - ADC7
*/
#include <avr/io.h>

#define __CONCATENATE(A, B) A##B
#define __CONCATENATE3(A, B, C) A##B##C
#define __EVALUATE(A, B) CONCATENATE(A, B)
#define __EVALUATE3(A, B, C) CONCATENATE3(A, B, C)

#define __MAKEPORT(P) __CONCATENATE(PORT, P)
#define __MAKEDDR(P) __CONCATENATE(DDR, P)

// Set pin as input
#define set_input_pin(PORT, PIN) (__MAKEDDR(PORT) &= ~_BV(PIN))

// Set pin as input with pull-up
#define set_input_pin_pull_up(PORT, PIN)                                       \
  ((__MAKEDDR(PORT) &= ~_BV(PIN)); (__MAKEPORT(PORT) |= _BV(PIN)))

// Set pin as output
#define set_output_pin(PORT, PIN) (__MAKEDDR(PORT) |= _BV(PIN))

// Set pin low or high
#define set_pin(PORT, PIN, STATE)                                              \
  if (STATE)                                                                   \
    __MAKEPORT(PORT) |= _BV(PIN);                                              \
  else                                                                         \
    __MAKEPORT(PORT) &= ~_BV(PIN);
