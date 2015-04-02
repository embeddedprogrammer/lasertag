#include "transmitter.h"
#include <stdint.h>
#include "supportFiles/buttons.h"
#include "supportFiles/switches.h"
#include "supportFiles/mio.h"
#include "supportFiles/utils.h"
#include <stdio.h>
#include "globals.h"
#include "supportFiles/mio.h"

#include "supportFiles/leds.h"
#include "supportFiles/interrupts.h"

#define PULSE_DURATION (200e-3 * GLOBALS_TIMER_FREQUENCY) // 200 ms
#define TRANSMITTER_LED_PIN 13 // JF 1

enum transmitter_states
	{initial_st, transmit_st} transmitter_state;

// Standard init function.
void transmitter_init()
{
	transmitter_state = initial_st;
	mio_setPinAsOutput(TRANSMITTER_LED_PIN);
}

static bool enabled;
static bool continuousMode;

// Starts the transmitter. Does nothing if the transmitter is already running.
void transmitter_run()
{
	enabled = true;
}

// Stops the transmitter.
void transmitter_stop()
{
	enabled = false;
}

// Returns true if the transmitter is running.
bool transmitter_running()
{
	return (enabled || transmitter_state == transmit_st);
}

uint16_t frequencyInHz;
uint16_t frequencyPlayerNumber;
const uint16_t frequencies[] = {1110, 1390, 1720, 2000, 2270, 2630, 2910, 3330, 3570, 3840};

// Sets the frequency number. If this function is called while the
// transmitter is running, the frequency will not be updated until the
// transmitter stops and transmitter_run() is called again.
void transmitter_setFrequencyNumber(uint16_t frequencyNumber)
{
	//frequencyInHz = frequencies[frequencyNumber];
	frequencyPlayerNumber = frequencyNumber;
}

uint16_t transmitter_getFrequencyNumber()
{
	return frequencyPlayerNumber;
}

static bool ledOn = false;

double transmitter_getLedOn()
{
	return ledOn ? 1.0 : 0.0;
}

int zzState = 0;

int transmitter_getState()
{
	return zzState;
}

int counter = 0;

int16_t frequency[] = {45, 36, 29, 25, 22, 19, 17, 15, 14, 13};

// Standard tick function.
void transmitter_tick()
{
	static int32_t pulseTimer;
	static int32_t periodTimer;
	static int32_t halfPeriodDuration;

	// Current state actions
	switch (transmitter_state)
	{
	case initial_st:
	break;
	case transmit_st:
		pulseTimer++;
		periodTimer++;
	break;
	}

	// State transition
	switch (transmitter_state)
	{
	case initial_st:
		if(enabled || continuousMode)
		{
			transmitter_state = transmit_st;
			halfPeriodDuration = frequency[frequencyPlayerNumber]; //(int32_t)(GLOBALS_TIMER_FREQUENCY / frequencyInHz / 2 + .5);
			pulseTimer = 0;
			periodTimer = 0;
		}
	break;
	case transmit_st:
		if(pulseTimer >= 20000) //PULSE_DURATION) //This macro for some reason causes data corruption. Float comparison?
		{
			zzState = 4;
			enabled = false;
			mio_writePin(TRANSMITTER_LED_PIN, 0);
			transmitter_state = initial_st;
		}
		else if(periodTimer >= halfPeriodDuration)
		{
			zzState = 5;
			periodTimer = 0;
			ledOn = !ledOn;
			mio_writePin(TRANSMITTER_LED_PIN, ledOn); // Turn on/off LED.
		}
	break;
	}
}

// Performs test of the hitLedTimer
void transmitter_runTest()
{
	transmitter_init();
	buttons_init();
	switches_init();
	mio_init(true);

	printf("Test transmitter. Each .5 seconds the transmitter frequency is\r\n");
	printf("set to the switch frequency. Press button 1 to exit test\r\n");
	u32 privateTimerTicksPerSecond = interrupts_getPrivateTimerTicksPerSecond();
	leds_init(true);
	interrupts_initAll(true);
	interrupts_enableTimerGlobalInts();
	interrupts_startArmPrivateTimer();
	interrupts_enableSysMonGlobalInts();
	interrupts_enableArmInts();
	while (true)
	{
		transmitter_setFrequencyNumber(switches_read());
		transmitter_run();
		utils_msDelay(500);
		if(buttons_read() & 0x2)
		{
			printf("Exiting\r\n");
			break;
		}
	}
    interrupts_disableArmInts();
}

void transmitter_setContinuousMode(bool mode)
{
	printf("Continuous mode\r\n");
	continuousMode = mode;
}
