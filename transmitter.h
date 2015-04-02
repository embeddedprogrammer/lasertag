#include <stdint.h>

#ifndef TRANSMITTER_H_
#define TRANSMITTER_H_

// Standard init function.
void transmitter_init();

// Starts the transmitter. Does nothing if the transmitter is already running.
void transmitter_run();

// Stops the transmitter.
void transmitter_stop();

// Returns true if the transmitter is running.
bool transmitter_running();

// Returns true if led is on.
double transmitter_getLedOn();

// Sets the frequency number. If this function is called while the
// transmitter is running, the frequency will not be updated until the
// transmitter stops and transmitter_run() is called again.
void transmitter_setFrequencyNumber(uint16_t frequencyNumber);

// Gets the frequency number
uint16_t transmitter_getFrequencyNumber();

int transmitter_getState();

// Standard tick function.
void transmitter_tick();

// Tests the transmitter.
void transmitter_runTest();

void transmitter_setContinuousMode(bool mode);

#endif /* TRANSMITTER_H_ */
