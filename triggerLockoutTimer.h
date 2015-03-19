#include <stdint.h>

#ifndef TRIGGER_LOCKOUT_TIMER_H_
#define TRIGGER_LOCKOUT_TIMER_H_

// Standard init function.
void triggerLockoutTimer_init();

// Calling this starts the timer.
void triggerLockoutTimer_start();

// Returns true if the timer is running.
bool triggerLockoutTimer_running();

// Standard tick function.
void triggerLockoutTimer_tick();

void triggerLockoutTimer_runTest();

#endif /* TRIGGER_LOCKOUT_TIMER_H_ */
