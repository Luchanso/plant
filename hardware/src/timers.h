#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Hardware timer wrapper for plant monitor

#include <etl/delegate.h>
#include <etl/memory.h>
#include <etl/singleton.h>
#include <stdint.h>

#ifndef PLANT_MONITOR_MAX_TIMERS
#define PLANT_MONITOR_MAX_TIMERS 8
#endif

enum timer_ids : uint8_t { usart_line_idle, one_second };

class timer_manager_instance {
  struct impl;
  etl::unique_ptr<impl> m_impl;

  timer_manager_instance();
  friend class etl::singleton<timer_manager_instance>;

public:
  using callback = etl::delegate<void(void)>;
  struct callback_timer {
    bool repeating = true;
    uint8_t id; // deliberately not set, possible randomness kinda favors it
    uint32_t ticks = 0;
    uint32_t timeout = 0;
    timer_manager_instance::callback callback;
    bool expired() { return ticks >= timeout; }
  };

  bool add_seconds_timer(callback_timer &&t);
  bool add_milliseconds_timer(callback_timer &&t);
  void remove_timer(const uint8_t id);
  void process_callbacks();

  void seconds_interrupt();
  void milliseconds_interrupt();
};

using timer_manager = etl::singleton<timer_manager_instance>;

