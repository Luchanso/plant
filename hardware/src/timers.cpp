#include <avr/interrupt.h>
#include <etl/algorithm.h>
#include <etl/list.h>
#include <etl/pool.h>

#include "timers.h"

/*
Refer to Atmega328p datasheet, section 15
https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
*/

/*
Together with prescaler value of 256, Timer 1 compare match A interrupt will
be triggered once a second, i.e. F_CPU / 256 / 62500 = 1;
where F_CPU usually equals to 16MHz for 5v Arduino Nano board with Atmega328p.
*/
#define OCR1A_CYCLES_FOR_ONE_SECOND (uint16_t)(F_CPU / 256)

/*
Same logic applies for 1 millisecond
*/
#define OCR1B_CYCLES_FOR_ONE_MILLISECOND (uint16_t)(1 / 1000 * F_CPU / 256 + 1)

struct timer_manager_instance::impl {
  using timer_list = etl::list_ext<callback_timer>;
  etl::pool<timer_list::pool_type, PLANT_MONITOR_MAX_TIMERS> m_pool;
  timer_list m_seconds_timers;
  timer_list m_milliseconds_timers;

  impl() {
    m_seconds_timers.set_pool(m_pool);
    m_milliseconds_timers.set_pool(m_pool);

    cli(); // stop interrupts

    TCCR1A = 0; // Clear TCCR1A register
    TCCR1B = 0; // Clear TCCR1B register
    TCNT1 = 0;  // Reset Timer CouNTer 1

    /*
    Adjust Timer 1 Control register B to set the prescaler to F_CPU/256.
    With F_CPU == 16MHz, this means, that every 16 microseconds value of TCNT1
    will be incremented by 1.
    */
    TCCR1B |= (1 << CS12);

    sei(); // allow interrupts
  }

  void enable_seconds_interrupt(bool enable) {
    if (enable) {
      /*
      The "Output Compare Register (Timer) 1 A" is used to trigger interrupt
      whenever OCR1A == TCNT1.
      */
      OCR1A = TCNT1 + OCR1A_CYCLES_FOR_ONE_SECOND;

      // Enable timer 1 output compare A match interrupt.
      TIMSK1 |= _BV(OCIE1A);
    } else {
      // Disable timer 1 output compare A match interrupt.
      TIMSK1 &= ~_BV(OCIE1A);
    }
  }

  bool seconds_interrupt_enabled() { return TIMSK1 & _BV(OCIE1A); }

  void enable_milliseconds_interrupt(bool enable) {
    if (enable) {
      // Enable timer 1 output compare B match interrupt
      OCR1B = TCNT1 + OCR1B_CYCLES_FOR_ONE_MILLISECOND;
      TIMSK1 |= _BV(OCIE1B);
    } else {
      TIMSK1 &= ~_BV(OCIE1B);
    }
  }

  bool milliseconds_interrupt_enabled() { return TIMSK1 & _BV(OCIE1B); }

  void second_tick() {
    OCR1A = TCNT1 + OCR1A_CYCLES_FOR_ONE_SECOND;
    for (auto &t : m_seconds_timers)
      ++t.ticks;
  }

  void millisecond_tick() {
    OCR1B = TCNT1 + OCR1B_CYCLES_FOR_ONE_MILLISECOND;
    for (auto &t : m_milliseconds_timers)
      ++t.ticks;
  }
};

timer_manager_instance::timer_manager_instance() : m_impl(new impl) {}

bool timer_manager_instance::add_seconds_timer(callback_timer &&t) {
  remove_timer(t.id);

  if (m_impl->m_pool.full())
    return false;

  m_impl->m_seconds_timers.emplace_back(t);

  if (!m_impl->seconds_interrupt_enabled())
    m_impl->enable_seconds_interrupt(true);

  return true;
}

bool timer_manager_instance::add_milliseconds_timer(callback_timer &&t) {
  remove_timer(t.id);

  if (m_impl->m_pool.full())
    return false;

  m_impl->m_milliseconds_timers.emplace_back(t);

  if (!m_impl->milliseconds_interrupt_enabled())
    m_impl->enable_milliseconds_interrupt(true);

  return true;
}

void timer_manager_instance::remove_timer(const uint8_t timer_id) {
  auto remove_by_id = [timer_id](const callback_timer &t) {
    return t.id == timer_id;
  };

  m_impl->m_seconds_timers.remove_if(remove_by_id);
  m_impl->m_milliseconds_timers.remove_if(remove_by_id);

  if (m_impl->m_seconds_timers.empty())
    m_impl->enable_seconds_interrupt(false);

  if (m_impl->m_milliseconds_timers.empty())
    m_impl->enable_milliseconds_interrupt(false);
}

void timer_manager_instance::process_callbacks() {
  auto callback_processor = [](auto &container) {
    auto iterator = container.begin();
    auto end = container.end();

    // For some reason, clang-format can't handle this. Disable for this loop.
    // clang-format off
    while (iterator != end) {
      if (iterator->expired()) {
      	iterator->callback();

      	if (iterator->repeating) {
      	  iterator->ticks = 0;
      	  ++iterator;
      	} else {
      	  iterator = container.erase(iterator);
				}
      } else {
        ++iterator;
      }
    }
    // clang-format on
  };

  callback_processor(m_impl->m_milliseconds_timers);
  callback_processor(m_impl->m_seconds_timers);

  if (m_impl->m_seconds_timers.empty())
    m_impl->enable_seconds_interrupt(false);

  if (m_impl->m_milliseconds_timers.empty())
    m_impl->enable_milliseconds_interrupt(false);
}

void timer_manager_instance::seconds_interrupt() { m_impl->second_tick(); }

void timer_manager_instance::milliseconds_interrupt() {
  m_impl->millisecond_tick();
}

ISR(TIMER1_COMPA_vect) { timer_manager::instance().seconds_interrupt(); }

ISR(TIMER1_COMPB_vect) { timer_manager::instance().milliseconds_interrupt(); }
