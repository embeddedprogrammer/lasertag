#include <stdint.h>

#ifndef HIT_LED_TIMER_H_
#define HIT_LED_TIMER_H_

// Need to init things.
void hitLedTimer_init();

// Calling this starts the timer.
void hitLedTimer_start();

// Returns true if the timer is currently running.
bool hitLedTimer_running();

// Standard tick function.
void hitLedTimer_tick();

// Performs test of the hitLedTimer
void hitLedTimer_runTest();

#endif /* HIT_LED_TIMER_H_ */
