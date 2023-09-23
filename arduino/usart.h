#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Use Atmega328p USART in a plant monitor specific way

#include "proto.h"

// Can be altered. Mind the hardware limitations of Atmega 328p!
#define BAUDRATE 9600

// Set the sizes of both the input and the output buffers, in bytes
#define USART_BUFFER_SIZE 32

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Initialize the Universal Synchronous/Asynchronous Receiver-Transmitter #0.
 * Must be executed once before sending and receiving data over USART.
 */
void pmUSARTInit();

/**
 * Send a plant monitor message.
 * @param message message to send.
 */
void pmUSARTSend(const plantMessage* message);

/**
 * Move data from the receiver buffer to heap-allocated memory.
 * @param[out] buffer pointer to heap-allocated copy of the buffer.
 * You MUST free it once you done with it. Won't be modified unless data is available.
 * @return size of the \p buffer in bytes.
 */
uint8_t pmUSARTGetReceivedData(uint8_t* buffer);

#if defined(__cplusplus)
}
#endif