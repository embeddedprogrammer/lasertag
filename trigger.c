#include "trigger.h"
#include "globals.h"
#include "triggerLockoutTimer.h"
#include "transmitter.h"
#include "supportFiles/buttons.h"
#include "supportFiles/mio.h"
#include <stdio.h>

#include "supportFiles/leds.h"
#include "supportFiles/switches.h"
#include "supportFiles/interrupts.h"


#define DEBOUNCE_DURATION (50e-3 * GLOBALS_TIMER_FREQUENCY) // 50 ms
#define TRIGGER_PIN 10         // JF 2
#define TRIGGER_BUTTON 0x1

enum trigger_states
	{wait_for_change_st, wait_till_steady_st} trigger_state;

bool ignoreGunInput;

// Init trigger data-structures.
void trigger_init()
{
	trigger_state = wait_for_change_st;
	mio_setPinAsInput(TRIGGER_PIN);
	ignoreGunInput = mio_readPin(TRIGGER_PIN);
	if(ignoreGunInput)
		printf("Gun input high - assuming it is disconnected\r\n");
	else
		printf("Gun input low - assuming it is connected\r\n");
}

static bool enabled;

// Enable the trigger state machine. The state-machine does nothing until it is enabled.
void trigger_enable()
{
	enabled = true;
	printf("enabled\r\n");
}

bool triggerPressed()
{
	return (buttons_read() & TRIGGER_BUTTON) || (!ignoreGunInput && mio_readPin(TRIGGER_PIN));
}

// Standard tick function.
void trigger_tick()
{
	static int32_t debounceTimer;
	static bool debouncedTriggerValue;

	// Current state actions
	switch (trigger_state)
	{
	case wait_for_change_st:
	break;
	case wait_till_steady_st:
		debounceTimer++;
		//printf("%d\r\n", debounceTimer);
	break;
	}

	bool currentTriggerValue = triggerPressed();

	// State transition
	switch (trigger_state)
	{
	case wait_for_change_st:
//		printf("%d, %d, %d\r\n", debouncedTriggerValue, currentTriggerValue, enabled);
		if(debouncedTriggerValue != currentTriggerValue && enabled)
		{
//			printf("changed\n");
			debounceTimer = 0;
			trigger_state = wait_till_steady_st;
		}
	break;
	case wait_till_steady_st:
		if(debouncedTriggerValue == currentTriggerValue)
		{
			trigger_state = wait_for_change_st;
		}
		else if(debounceTimer >= 5000) //DEBOUNCE_DURATION)
		{
			debouncedTriggerValue = currentTriggerValue;
//			if(debouncedTriggerValue)
//				printf("D\r\n");
//			else
//				printf("U\r\n");
			if(debouncedTriggerValue && !triggerLockoutTimer_running())
			{
				transmitter_run();
				triggerLockoutTimer_start();
			}
			trigger_state = wait_for_change_st;
		}
	break;
	}
}

// Performs test of the hitLedTimer
void trigger_runTest()
{
	trigger_init();
	trigger_enable();
	transmitter_init();
	buttons_init();
	triggerLockoutTimer_init();
	switches_init();
	mio_init(true);
	transmitter_setFrequencyNumber(switches_read());

	printf("Transmitter frequency is set to the switch frequency. Press trigger to test, button 1 to exit\r\n");
	u32 privateTimerTicksPerSecond = interrupts_getPrivateTimerTicksPerSecond();
	leds_init(true);
	interrupts_initAll(true);
	interrupts_enableTimerGlobalInts();
	interrupts_startArmPrivateTimer();
	interrupts_enableSysMonGlobalInts();
	interrupts_enableArmInts();
	while (true)
	{
		if (interrupts_isrFlagGlobal)
		{
			interrupts_isrFlagGlobal = 0;
			if(buttons_read() & 0x2)
			{
				printf("Exiting\r\n");
				break;
			}
		}
	}
    interrupts_disableArmInts();
}
