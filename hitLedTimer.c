#include "hitLedTimer.h"
#include "globals.h"
#include "supportFiles/mio.h"

#include "supportFiles/buttons.h"
#include "supportFiles/leds.h"
#include "supportFiles/interrupts.h"
#include "supportFiles/utils.h"
#include "supportFiles/leds.h"
#include "stdio.h"


#define LED_ON_DURATION (.5 * GLOBALS_TIMER_FREQUENCY) // .5 s
#define HIT_INDICATOR_PIN 11   // JF 3

enum hitLedTimer_states
	{ledOff_st, ledOn_st} hitLedTimer_state;

// Need to init things.
void hitLedTimer_init()
{
	leds_init(true);
	hitLedTimer_state = ledOff_st;
	mio_setPinAsOutput(HIT_INDICATOR_PIN);
}

static bool enabled;

// Calling this starts the timer.
void hitLedTimer_start()
{
	enabled = true;
}

// Returns true if the timer is currently running.
bool hitLedTimer_running()
{
	return (enabled || hitLedTimer_state == ledOn_st);
}

// Standard tick function. Modify in future to play a song.
void hitLedTimer_tick()
{
	static int32_t ledTimer;

	// Current state actions
	switch (hitLedTimer_state)
	{
	case ledOff_st:
	break;
	case ledOn_st:
		ledTimer++;
	break;
	}

	// State transition
	switch (hitLedTimer_state)
	{
	case ledOff_st:
		if(enabled)
		{
			hitLedTimer_state = ledOn_st;
			ledTimer = 0;
			mio_writePin(HIT_INDICATOR_PIN, 1);
			leds_write(0x1);
		}
	break;
	case ledOn_st:
		if(ledTimer >= LED_ON_DURATION)
		{
			hitLedTimer_state = ledOff_st;
			mio_writePin(HIT_INDICATOR_PIN, 0);
			leds_write(0x0);
			enabled = false;
		}
	break;
	}
}

// Performs test of the hitLedTimer
void hitLedTimer_runTest()
{
	hitLedTimer_init();
	buttons_init();

	printf("Test hitLedTimer. Press button 0 to start hit led, button 1 to stop test\r\n");
	u32 privateTimerTicksPerSecond = interrupts_getPrivateTimerTicksPerSecond();
	leds_init(true);
	interrupts_initAll(true);
	interrupts_enableTimerGlobalInts();
	interrupts_startArmPrivateTimer();
	interrupts_enableSysMonGlobalInts();
	interrupts_enableArmInts();
	while (true)
	{
		hitLedTimer_start();
		int32_t buttonsVal;
		while(hitLedTimer_running())
		{
			buttonsVal = buttons_read();
			if(buttonsVal & 0x2)
				break;
		}

		if(buttonsVal & 0x2)
			break;
		utils_msDelay(100);
	}
	printf("Exiting\r\n");
    interrupts_disableArmInts();
}

//// Performs test of the hitLedTimer
//void hitLedTimer_runTest()
//{
//	hitLedTimer_init();
//	buttons_init();
//
//	printf("Test hitLedTimer. Press button 0 to start hit led, button 1 to stop test\r\n");
//	u32 privateTimerTicksPerSecond = interrupts_getPrivateTimerTicksPerSecond();
//	leds_init(true);
//	interrupts_initAll(true);
//	interrupts_enableTimerGlobalInts();
//	interrupts_startArmPrivateTimer();
//	interrupts_enableSysMonGlobalInts();
//	interrupts_enableArmInts();
//	while (true)
//	{
//		if (interrupts_isrFlagGlobal)
//		{
//			interrupts_isrFlagGlobal = 0;
//			hitLedTimer_tick();
//
//			int32_t buttonsVal = buttons_read();
//			if((buttonsVal & 0x1) && !hitLedTimer_running())
//			{
//				printf("Starting hitLedTimer\r\n");
//				hitLedTimer_start();
//			}
//
//			if(buttonsVal & 0x2)
//			{
//				printf("Exiting\r\n");
//				break;
//			}
//		}
//	}
//    interrupts_disableArmInts();
//}

