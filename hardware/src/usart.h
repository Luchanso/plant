#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Use Atmega328p USART in a plant monitor specific way

#include "proto.h"

// Set sizes of both the input and the output buffers, in bytes
#define USART_BUFFER_SIZE 32

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Initialize the Universal Synchronous/Asynchronous Receiver-Transmitter #0.
 * Must be executed once before sending and receiving data over USART.
 * The baudrate is set to 115200
 */
void pmUSARTInit();

/**
 * Send a plant monitor message asynchroniosly.
 * @param message message to send.
 */
void pmUSARTSend(const plantMessage *message);

/**
 * Copy data from the receiver buffer to heap-allocated memory.
 * @param[out] buffer pointer to heap-allocated copy of the buffer.
 * You MUST free it once you done with it. Won't be modified unless data is
 * available.
 * @return size of the @p buffer in bytes.
 */
uint8_t pmUSARTCopyReceivedData(uint8_t **buffer);

/**
 * Clear the receiver buffer.
 */
void pmUSARTClearRxBuffer();

/**
 * Send null-terminated debug string over serial using blocking I/O.
 * Since this function will block until the entire message is sent, it's not
 * recommended to use this function for anything else but debugging.
 * @param message null-terminated string to send.
 */
void pmUSARTSendDebugText(const char *message);

/**
 * Send decimal debug number as string of ASCII characters over serial using
 * blocking I/O. Since this function will block until the entire message is
 * sent, it's not recommended to use this function for anything else but
 * debugging.
 * @param number number to send.
 */
void pmUSARTSendDebugNumber(int32_t number);

#if defined(__cplusplus)
}
#endif
