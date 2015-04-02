/*
 * playTone.c
 *
 *  Created on: Apr 2, 2015
 *      Author: samuelwg
 */

#include "playTone.h"

bool playTone_enable;
uint32_t playTone_numTicks, playTone_totalTicks;
uint32_t currentNumTicks, currentTotalTicks;

enum playTone_states {init_st, high_st, low_st} playTone_state = init_st;

void playTone_init(){
	playTone_enable = false;
	mio_init(false);
	mio_setPinAsOutput(SOUND_OUTPUT_PIN);
	mio_writePin(SOUND_OUTPUT_PIN, 0);
}

void playTone_play(uint32_t numTicks, uint32_t totalTicks){
	playTone_enable = true;
	playTone_numTicks = numTicks;
	playTone_totalTicks = totalTicks;
}

bool playTone_done(){
	return !playTone_enable;
}

void playTone_tick(){
	//state actions
	switch(playTone_state){
		case init_st:
			break;
		case high_st:
			currentNumTicks++;
			currentTotalTicks++;
			break;
		case low_st:
			currentNumTicks++;
			currentTotalTicks++;
			break;
		default:
			break;
	}
	//state transitions
	switch(playTone_state){
		case init_st:
			if(playTone_enable){
				currentNumTicks = 0;
				currentTotalTicks = 0;
				mio_writePin(SOUND_OUTPUT_PIN, 1);
				playTone_state = high_st;
			}
			else{
				playTone_state = init_st;
			}
			break;
		case high_st:
			if(currentTotalTicks >= playTone_totalTicks){
				mio_writePin(SOUND_OUTPUT_PIN, 0);
				playTone_enable = false;
//				printf("Play tone: Entering init state\n\r");
				playTone_state = init_st;
			}
			else if(currentNumTicks < playTone_numTicks){
				playTone_state = high_st;
			}
			else{
				currentNumTicks = 0;
				mio_writePin(SOUND_OUTPUT_PIN, 0);
				playTone_state = low_st;
			}
			break;
		case low_st:
			if(currentTotalTicks >= playTone_totalTicks){
				mio_writePin(SOUND_OUTPUT_PIN, 0);
				playTone_enable = false;
//				printf("Play tone: Entering init state\n\r");
				playTone_state = init_st;
			}
			else if(currentNumTicks < playTone_numTicks){
				playTone_state = low_st;
			}
			else{
				currentNumTicks = 0;
				mio_writePin(SOUND_OUTPUT_PIN, 1);
				playTone_state = high_st;
			}
			break;
		default:
			playTone_enable = false;
			playTone_state = init_st;
	}
}

void playTone_runTest(){
	playTone_init();

	// We want to use the interval timers.
	intervalTimer_initAll();
	intervalTimer_resetAll();
	intervalTimer_testAll();
	printf("Transmitter Test Program\n\r");
	// Find out how many timer ticks per second.
	u32 privateTimerTicksPerSecond = interrupts_getPrivateTimerTicksPerSecond();
	printf("private timer ticks per second: %ld\n\r", privateTimerTicksPerSecond);
	// Initialize the GPIO LED driver and print out an error message if it fails (argument = true).
	// The LD4 LED provides a heart-beat that visually verifies that interrupts are running.
	leds_init(true);
	// Init all interrupts (but does not transmitter_enable the interrupts at the devices).
	// Prints an error message if an internal failure occurs because the argument = true.
	interrupts_initAll(true);
	// Enables the main interrupt on the time.
	interrupts_enableTimerGlobalInts();
	// Start the private ARM timer running.
	interrupts_startArmPrivateTimer();
	printf("This program will run for 10 seconds.\n\r");
	printf("Starting timer interrupts.\n\r");
	// transmitter_enable global interrupt of System Monitor. The system monitor contains the ADC. Mainly to detect EOC interrupts.
	interrupts_enableSysMonGlobalInts();
	// Start a duration timer and compare its timing against the time computed by the timerIsr().
	// Assume that ENABLE_INTERVAL_TIMER_0_IN_TIMER_ISR is defined in interrupts.h so that time spent in timer-isr is measured.
	intervalTimer_start(1);
	int countInterruptsViaInterruptsIsrFlag = 0;
	// enable interrupts at the ARM.
	interrupts_enableArmInts();
	// Wait until TOTAL seconds have expired. globalTimerTickCount is incremented by timer isr.
	while (interrupts_isrInvocationCount() < (PLAY_TONE_TOTAL_SECONDS * privateTimerTicksPerSecond)) {
		playTone_play(50, 1000000);
		while(!playTone_done()){
			if (interrupts_isrFlagGlobal) {				// If this is true, an interrupt has occured (at least one).
				countInterruptsViaInterruptsIsrFlag++;	// Note that you saw it.
				interrupts_isrFlagGlobal = 0;			// Reset the shared flag.
				playTone_tick();
			}
		}
		utils_msDelay(500);//wait for 500 ms
	}
	interrupts_disableArmInts();	// Disable ARM interrupts.
	intervalTimer_stop(1);			// Stop the interval timer

	double runningSeconds, isrRunningSeconds;
	intervalTimer_getTotalDurationInSeconds(1, &runningSeconds);
	printf("Total run time as measured by interval timer in seconds: %g.\n\r", runningSeconds);
	intervalTimer_getTotalDurationInSeconds(0, &isrRunningSeconds);
	printf("Measured run time in timerIsr (using interval timer): %g.\n\r", isrRunningSeconds);
	printf("Detected interrupts via global flag: %d\n\r", countInterruptsViaInterruptsIsrFlag);
}




