#include <stdint.h>

#ifndef TRIGGER_H_
#define TRIGGER_H_

// Init trigger data-structures.
void trigger_init();

// Enable the trigger state machine. The state-machine does nothing until it is enabled.
void trigger_enable();

// Standard tick function.
void trigger_tick();

void trigger_runTest();

#endif /* TRIGGER_H_ */
