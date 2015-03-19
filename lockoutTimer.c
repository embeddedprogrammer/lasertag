#include "lockoutTimer.h"
#include "globals.h"
#include "stdio.h"

#include "supportFiles/leds.h"
#include "supportFiles/interrupts.h"
#include "supportFiles/buttons.h"
#include "supportFiles/intervalTimer.h"
#include "supportFiles/utils.h"

#define LOCKOUT_DURATION (.5 * GLOBALS_TIMER_FREQUENCY) // .5 s

enum lockoutTimer_states
	{wait_st, lockout_st} lockoutTimer_state;

// Standard init function.
void lockoutTimer_init()
{
	lockoutTimer_state = wait_st;
}

static bool enabled;

// Calling this starts the timer.
void lockoutTimer_start()
{
	enabled = true;
}

static int32_t lockoutTimer;

// Returns true if the timer is running.
bool lockoutTimer_running()
{
	return (enabled || (lockoutTimer_state == lockout_st));
}

// Standard tick function.
void lockoutTimer_tick()
{
	// Current state actions
	switch (lockoutTimer_state)
	{
	case wait_st:
	break;
	case lockout_st:
		lockoutTimer++;
	break;
	}

	// State transition
	switch (lockoutTimer_state)
	{
	case wait_st:
		if(enabled)
		{
			lockoutTimer_state = lockout_st;
			lockoutTimer = 0;
		}
	break;
	case lockout_st:
		if(lockoutTimer >= LOCKOUT_DURATION)
		{
			lockoutTimer_state = wait_st;
			enabled = false;
		}
	break;
	}
}

// Performs test of the hitLedTimer
void lockoutTimer_runTest()
{
	buttons_init();
	lockoutTimer_init();
	intervalTimer_init(1);

	printf("Test lockoutTimer. Press button 1 to exit\r\n");
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
		lockoutTimer_start();
		intervalTimer_reset(1);
		intervalTimer_start(1);
		int32_t buttonsVal;
		while(lockoutTimer_running())
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
