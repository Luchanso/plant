#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// USART
#include <etl/circular_buffer.h>
#include <etl/fsm.h>

#include "convert_util.h"
#include "proto.h"

// Set size of the output buffer, in bytes
#define USART_BUFFER_SIZE 32

#ifdef DEAD_CODE
class usart {
  using message = etl::unique_ptr<ibytevect>;

  ibytevect::iterator m_output_iterator = nullptr;
  message m_output_buffer;
  etl::circular_buffer<uint8_t, USART_BUFFER_SIZE> m_input_buffer;

public:
  usart(const uint32_t &baudrate = 115200);

  /**
   * Set transmission speed.
   * @param baudrate speed to set
   * @return true, if baudrate succesfully applied
   */
  bool set_baudrate(uint32_t baudrate);

  /**
   * Send data over USART.
   * @param msg message to send. NOTE: takes ownership of a message.
   */
  void send(message &&msg);

  /**
   * Check if USART is busy sending or receiving data.
   * @return true if USART is busy.
   */
  bool busy();

  /**
   * Get the amount of bytes in incoming buffer.
   * @return amount of bytes.
   */
  uint8_t bytes_received();

  ibytevect get_received_data();

  /**
   * Send null-terminated debug string over serial using blocking I/O.
   * Since this function will block until the entire message is sent, it's not
   * recommended to use this function for anything else but debugging.
   * @param message null-terminated string to send.
   */
  void send_debug_text(const char *str);

  /**
   * Send decimal debug number as string of ASCII characters over serial using
   * blocking I/O. Since this function will block until the entire message is
   * sent, it's not recommended to use this function for anything else but
   * debugging.
   * @param number number to send.
   */
  void send_debug_number(const int32_t &val);
};
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Serial line idle callback.
 */
extern void (*pmUSARTLineIdleCallback)(void);

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
