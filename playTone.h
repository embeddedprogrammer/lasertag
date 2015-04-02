/*
 * playtone.h
 *
 *  Created on: Apr 2, 2015
 *      Author: samuelwg
 */

#ifndef PLAYTONE_H_
#define PLAYTONE_H_

#include <stdint.h>
#include "supportFiles/mio.h"
#include <stdbool.h>
#include "supportFiles/leds.h"
#include "supportFiles/globalTimer.h"
#include "supportFiles/interrupts.h"
#include "supportFiles/intervalTimer.h"
#include "supportFiles/utils.h"
#include <stdio.h>

#define SOUND_OUTPUT_PIN 12
#define PLAY_TONE_TOTAL_SECONDS 10

//basic init function
void playTone_init();

//play a tone at the desired frequency (based on calculation numTicks for 100 kHz sample rate) and duration (totalTicks)
void playTone_play(uint32_t numTicks, uint32_t totalTicks);

//returns whether the tone has finished playing
bool playTone_done();

//tick function
void playTone_tick();

void playTone_runTest();

#endif /* PLAYTONE_H_ */
