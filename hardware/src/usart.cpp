#include <avr/interrupt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proto.h"
#include "usart.h"

#define __PLANT_MESSAGE_STRUCT
#include "plant_message_struct.h"

#include "timers.h"

/*
Refer to Atmega328p datasheet, section 19:
https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
*/

/*
Using the table 19-12 of the datasheet, select value for baudrate 115200
*/
#define UBRRVAL 8

#ifdef DEAD_CODE
usart::usart(const uint32_t &baudrate) {

  set_baudrate(baudrate);

  /*
  Using USART 0 Control and Status Register 0 C, set operation mode:
  Asynchronous 8 bits per frame, No parity, 1 stop bit
  UMSEL00 and UMSEL01 both set to zero = asynchronous USART mode
  UPM00 and UPM01 both set to zero = no parity
  USBS0 set to zero = 1 stop bit
  UCSZ00 and UCSZ01 set to one whlie UCSZ02 is set to zero = 8 bits per frame
  */
  UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);

  /*
  Using USART 0 Control and Status Register 0 B (U C S R 0 B), enable:
    - Recieve Complete Interrupt 0 (RX C I E 0)
    - Transmit Complete Interrupt 0 (TX C I E 0)
    - USART 0 Transmitter (TX EN 0)
    - USART 0 Receiver (RX EN 0)
  */
  UCSR0B |= (1 << RXCIE0) | (1 << TXCIE0) | (1 << TXEN0) | (1 << RXEN0);
}

bool usart::set_baudrate(uint32_t baudrate) {
  /*
  Enable the double transmission speed. It is always preferable to have this
  enabled, as it usually means more accurate baudrate and higher maximum
  transmission speed.
  */
  UCSR0A |= _BV(U2X0);

  /*
  Table 19.1 of the datasheet gives the formula for a baudrate calculation:
  baudrate = F_CPU / (8 (UBRR + 1))
  Solving the formula above for the UBRR gives this formula:
  UBRR = F_CPU / baudrate / 8  - 1
  */
  baudrate = ((uint32_t)F_CPU / baudrate / 8 - 1);

  /*
  UBRR prescaler has the capacity of 8 bits. If the baudrate exceeds this limit,
  it is unachievable.
  */
  if (baudrate > UINT8_MAX)
    return false;

  UBRR0 = (uint8_t)baudrate;

  return true;
}
#endif

// Dump all received bytes here
volatile uint8_t rxBuffer[USART_BUFFER_SIZE] = {};

// Put all data to be sent here
volatile uint8_t txBuffer[USART_BUFFER_SIZE] = {};

static volatile uint8_t bytesReceived = 0;
static volatile uint8_t bytesToSend = 0;
static volatile uint8_t bytesSent = 0;

void (*pmUSARTLineIdleCallback)(void) = nullptr;

void pmUSARTInit() {
  /*
  Set the Baud rate using USART Baud Rate Register 0
  */
  UBRR0 = UBRRVAL;

  /*
  Using USART 0 Control and Status Register 0 C, set operation mode:
  Asynchronous 8 bits per frame, No parity, 1 stop bit
  UMSEL00 and UMSEL01 both set to zero = asynchronous USART mode
  UPM00 and UPM01 both set to zero = no parity
  USBS0 set to zero = 1 stop bit
  UCSZ00 and UCSZ01 set to one whlie UCSZ02 is set to zero = 8 bits per frame
  */
  UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);

  /*
  Using USART 0 Control and Status Register 0 B (U C S R 0 B), enable:
    - Recieve Complete Interrupt 0 (RX C I E 0)
    - Transmit Complete Interrupt 0 (TX C I E 0)
    - USART 0 Transmitter (TX EN 0)
    - USART 0 Receiver (RX EN 0)
  */
  UCSR0B |= (1 << RXCIE0) | (1 << TXCIE0) | (1 << TXEN0) | (1 << RXEN0);
}

void pmUSARTSend(const plantMessage *message) {
  bytesToSend = USART_BUFFER_SIZE;
  pmSerialize(message, (uint8_t *)txBuffer, (uint8_t *)&bytesToSend);

  // Send the first byte straight away. The rest of them will be sent in TX ISR
  UDR0 = txBuffer[0];
  bytesSent = 1;
}

uint8_t pmUSARTCopyReceivedData(uint8_t **buffer) {
  if (!bytesReceived)
    return 0;

  *buffer = static_cast<uint8_t *>(malloc(bytesReceived));
  memcpy(*buffer, (const uint8_t *)rxBuffer, bytesReceived);

  return bytesReceived;
}

void pmUSARTClearRxBuffer() { bytesReceived = 0; }

// Succesfully received one frame - stash it, or, if buffer is full, dispose it.
ISR(USART_RX_vect) {
  cli();
  if (bytesReceived < USART_BUFFER_SIZE) {
    rxBuffer[bytesReceived] = UDR0;
    ++bytesReceived;

    timer_manager_instance::callback_timer t;
    t.timeout = 100;
    t.id = timer_ids::usart_line_idle;
    t.repeating = false;
    t.callback = []() {
      if (pmUSARTLineIdleCallback)
        pmUSARTLineIdleCallback();
    };

    timer_manager::instance().add_milliseconds_timer(etl::move(t));
  }
  sei();
}

// Succesfully transmitted one frame - transmit the next one if required.
ISR(USART_TX_vect) {
  if (bytesSent != bytesToSend) {
    UDR0 = txBuffer[bytesSent];
    ++bytesSent;
  }
}

void pmUSARTSendDebugText(const char *message) {
  UCSR0B &= ~((1 << RXCIE0) | (1 << TXCIE0));

  while (*message) {
    // Wait for USART Data Register 0 to become empty before writing anything.
    while (!(UCSR0A & (1 << UDRE0)))
      ;
    UDR0 = *message;
    ++message;
  }

  UCSR0B |= (1 << RXCIE0) | (1 << TXCIE0);
}

void pmUSARTSendDebugNumber(int32_t number) {
  // int32 will have at most 12 digits, including '-' and '\0'
  char *buffer = static_cast<char *>(malloc(12));
  snprintf(buffer, 12, "%ld", number);
  pmUSARTSendDebugText(buffer);
  free(buffer);
}
