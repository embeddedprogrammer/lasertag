#include "triggerLockoutTimer.h"
#include "globals.h"
#include "stdio.h"

#include "supportFiles/leds.h"
#include "supportFiles/interrupts.h"
#include "supportFiles/buttons.h"
#include "supportFiles/intervalTimer.h"
#include "supportFiles/utils.h"

#define LOCKOUT_DURATION (.5 * GLOBALS_TIMER_FREQUENCY) // .5 s

enum triggerLockoutTimer_states
	{wait_st, lockout_st} triggerLockoutTimer_state;

// Standard init function.
void triggerLockoutTimer_init()
{
	triggerLockoutTimer_state = wait_st;
}

static bool enabled;

// Calling this starts the timer.
void triggerLockoutTimer_start()
{
	enabled = true;
}

static int32_t triggerLockoutTimer;

// Returns true if the timer is running.
bool triggerLockoutTimer_running()
{
	return (enabled || (triggerLockoutTimer_state == lockout_st));
}

// Standard tick function.
void triggerLockoutTimer_tick()
{
	// Current state actions
	switch (triggerLockoutTimer_state)
	{
	case wait_st:
	break;
	case lockout_st:
		triggerLockoutTimer++;
	break;
	}

	// State transition
	switch (triggerLockoutTimer_state)
	{
	case wait_st:
		if(enabled)
		{
			triggerLockoutTimer_state = lockout_st;
			triggerLockoutTimer = 0;
		}
	break;
	case lockout_st:
		if(triggerLockoutTimer >= LOCKOUT_DURATION)
		{
			triggerLockoutTimer_state = wait_st;
			enabled = false;
		}
	break;
	}
}

// Performs test of the hitLedTimer
void triggerLockoutTimer_runTest()
{
	buttons_init();
	triggerLockoutTimer_init();
	intervalTimer_init(1);

	printf("Test triggerLockoutTimer. Press button 1 to exit\r\n");
	u32 privateTimerTicksPerSecond = interrupts_getPrivateTimerTicksPerSecond();
	leds_init(true);
	interrupts_initAll(true);
	interrupts_enableTimerGlobalInts();
	interrupts_startArmPrivateTimer();
	interrupts_enableSysMonGlobalInts();
	interrupts_enableArmInts();
	bool wasLocked = false;
	while (true)
	{
		triggerLockoutTimer_start();
		intervalTimer_reset(1);
		intervalTimer_start(1);
		int32_t buttonsVal;
		while(triggerLockoutTimer_running())
		{
			buttonsVal = buttons_read();
			if(buttonsVal & 0x2)
				break;
		}
		if(buttonsVal & 0x2)
			break;
		intervalTimer_stop(1);
		double seconds;
		intervalTimer_getTotalDurationInSeconds(1, &seconds);
		printf("Lockout timer ran for %g seconds\r\n", seconds);
		utils_msDelay(100);
	}
	printf("Exited\r\n");
    interrupts_disableArmInts();
}
