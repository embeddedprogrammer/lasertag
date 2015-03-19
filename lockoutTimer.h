#include <stdint.h>

#ifndef LOCKOUT_TIMER_H_
#define LOCKOUT_TIMER_H_

// Standard init function.
void lockoutTimer_init();

// Calling this starts the timer.
void lockoutTimer_start();

// Returns true if the timer is running.
bool lockoutTimer_running();

// Standard tick function.
void lockoutTimer_tick();

void lockoutTimer_runTest();

#endif /* LOCKOUT_TIMER_H_ */
