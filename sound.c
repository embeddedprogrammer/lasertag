/*
 * sound.c
 *
 *  Created on: Mar 31, 2015
 *      Author: kirkma1
 */

#include "sound.h"
#include "transmitter.h"
#include "hitLedtimer.h"
#include "playTone.h"


#define FREQ_ARRAY_SIZE 10
#define HIT_TOTAL_TIMER_EXPIRED 50000
#define SHOOT_TOTAL_TIMER_EXPIRED 20000
#define SHOOT_TONE_DURATION 2000
#define HIT_TONE_DURATION 5000

static uint16_t freq_ticks[FREQ_ARRAY_SIZE] = {500,250,167,125,100,83,71,63,56,50};
static uint16_t index = 0;
static uint16_t totalTimer = 0;
static uint16_t totalTimer_expired_value = 0;


enum sound_st_t {
	init_st,
	hit_st,
	shoot_st
} sound_currentState = init_st;

void sound_debugStatePrint() {
	static sound_st_t previousState;
	static bool firstPass = true;
	// Only print the message if:
	// 1. This the first pass and the value for previousState is unknown.
	// 2. previousState != currentState - this prevents reprinting the same state name over and over.
	if (previousState != sound_currentState || firstPass) {
		firstPass = false;                // previousState will be defined, firstPass is false.
		previousState = sound_currentState;     // keep track of the last state that you were in.
		//    printf("Led Counter:%d\n\r", LED_counter);
		switch(sound_currentState) {            // This prints messages based upon the state that you were in.
		case init_st:
			printf("init_st\n\r");
			break;
		case hit_st:
			printf("hit_st\n\r");
			break;
		case shoot_st:
			printf("shoot_st\n\r");
			break;
		}
	}
}


void sound_tick(){
	switch(sound_currentState) {
	case init_st:

		break;
	case hit_st:
		totalTimer++;
		break;
	case shoot_st:
		totalTimer++;
		break;
	default:
		printf("Shoot_tick state action: hit default\n\r");
		break;
	}

	// Transition states
	switch(sound_currentState) {
	case init_st:
		if(transmitter_enable()){
			index = FREQ_ARRAY_SIZE - 1;
			totalTimer = 0;
			totalTimer_expired_value = SHOOT_TOTAL_TIMER_EXPIRED;
			sound_currentState = shoot_st;
		}else if(hitLedTimer_running()){
			index = 0;
			totalTimer = 0;
			totalTimer_expired_value = HIT_TOTAL_TIMER_EXPIRED;
			sound_currentState = hit_st;
		}
		break;
	case hit_st:
		if(playTone_done() && (totalTimer < totalTimer_expired_value)){
			index++;
			playTone_play(freq_ticks[index], HIT_TONE_DURATION);
		}else if(totalTimer == totalTimer_expired_value){
			sound_currentState = init_st;
		}


		break;
	case shoot_st:
		if(playTone_done() && (totalTimer < totalTimer_expired_value)){
			index--;
			playTone_play(freq_ticks[index], SHOOT_TONE_DURATION);
		}else if(totalTimer == totalTimer_expired_value){
			sound_currentState = init_st;
		}
		break;

	default:
		printf("Sound_tick state update: hit default\n\r");
		break;
	}


}

void sound_init(){


}
