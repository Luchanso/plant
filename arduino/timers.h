#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Use Atmega328p hardware timer interrupts for plant monitor

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Init hardwire timer 1 and COMPA/COMPB interrupts.
 * @param OneSecondCallback this callback will be called every second as long as one second timer is running.
 * @param LineIdleCallback this callback be called after 3.5 characters of silence on receiving side of USART.
 */
void pmSetupTimersInterrupts(void (*OneSecondCallback)(), void (*LineIdleCallback)());

/*
 * Enable the timer 1 interrupt each second.
 */
void pmStartOneSecondTimer();

/*
 * Disable the timer 1 interrupt each second.
 */
void pmStopOneSecondTimer();

#if defined(__cplusplus)
}
#endif