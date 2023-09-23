#include "usart.h"
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#include "proto.h"

#define __PLANT_MESSAGE_STRUCT
#include "plant_message_struct.h"

#define __PLANT_MESSAGE_USART_TIMER
#include "timer_usart.h"

/*
Refer to Atmega328p datasheet, section 19
https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
*/

/*
Using the handy formula, provided in the section 19.3.1 of the datasheet,
in the table 19-1, calculate the USART Baud Rate Register value.
*/
#define UBRRVAL ((F_CPU / (BAUDRATE * 16UL)) - 1)

// Dump all received bytes here
static volatile uint8_t rxBuffer[USART_BUFFER_SIZE] = {};

// Put all data to be sent here
static volatile uint8_t txBuffer[USART_BUFFER_SIZE] = {};

static volatile uint8_t bytesReceived = 0;
static volatile uint8_t bytesToSend = 0;
static volatile uint8_t bytesSent = 0;

void pmUSARTInit() {
  cli();
  /*
  Set the Baud rate using USART Baud Rate Register 0
  (that register has the size of 12 bits, hence the 0xFFF mask)
  */
  UBRR0 = UBRRVAL & 0xFFF;

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

  sei();
}

void pmUSARTSend(const plantMessage* message) {
  bytesToSend = USART_BUFFER_SIZE;
  pmSerialize(message, (uint8_t*)txBuffer, (uint8_t*)&bytesToSend);

  // Send the first byte straight away. The rest of them will be sent in TX ISR
  UDR0 = txBuffer[0];
  bytesSent = 1;
}

uint8_t pmUSARTGetReceivedData(uint8_t* buffer) {
  if (!bytesReceived)
    return 0;

  uint8_t bufferSize = bytesReceived;
  buffer = malloc(bytesReceived);
  memcpy(buffer, (const uint8_t*)rxBuffer, bufferSize);
  bytesReceived = 0;

  return bufferSize;
}

// Succesfully received one frame - stash it, or, if buffer is full, dispose it.
ISR(USART_RX_vect) {
  if (bytesReceived < USART_BUFFER_SIZE) {
    rxBuffer[bytesReceived] = UDR0;
    ++bytesReceived;
    pmStartSerialLineIdleTimer();
  }
}

// Succesfully transmitted one frame - transmit the next one if required.
ISR(USART_TX_vect) {
  if (bytesSent != bytesToSend) {
    UDR0 = txBuffer[bytesSent];
    ++bytesSent;
  }
}
