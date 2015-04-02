/*
 * detector.c
 *
 *  Created on: Dec 22, 2014
 *      Author: hutch
 */

#include "supportFiles/interrupts.h"
#include <stdint.h>
#include "isr.h"
#include "stdio.h"
#include "filter.h"
#include "lockoutTimer.h"
#include "detector.h"
#include "hitLedTimer.h"

#define FUDGE_FACTOR 100
#define TRANSMITTER_TICK_MULTIPLIER 3	// Call the tick function this many times for each ADC interrupt.

int sampleCount = 0;
int sampleCount2 = 0;
int debugCount = 0;
bool hitDetected = false;

double sortedPowerValues[10];
int correspondingPlayers[10];
detector_hitCount_t hitCounts[10];

// Always have to init things.
void detector_init()
{
	//filter_init(); //Init already in another place!
}

// This runs the entire detector once each time 10 new inputs have been received, including the decimating
// FIR-filter, all 10 IIR-filters, power computation, and the previously-described hit-detection algorithm.
// detector() sets a boolean ! flag to true if a hit was detected.
void detector()
{
	while(isr_adcBufferElementCount() > 0)
	{
		interrupts_disableArmInts();
		uint32_t rawAdcValue = isr_removeDataFromAdcBuffer();
		interrupts_enableArmInts();
		double scaledAdcValue = ((double)rawAdcValue) / 4096;
		filter_addNewInput(scaledAdcValue);
		sampleCount++;
		if(sampleCount == FILTER_FIR_DECIMATION_FACTOR)
		{
			sampleCount = 0;
			sampleCount2++;
			bool debug = false;
			if(sampleCount2 == 10000)
			{
				sampleCount2 = 0;
				debug = true;
			}
			filter_firFilter();
			if(debug)
			{
				printf("Power values: ");
			}
			for(int filterNumber = 0; filterNumber < FILTER_IIR_FILTER_COUNT; filterNumber++)
			{
				filter_iirFilter(filterNumber);
				double power = filter_computePower(filterNumber, false, false);
				if(debug)
					printf("%9.2e ", power);
			}
			if(debug)
				printf("\r\n");
			filter_getSortedPowerValues(sortedPowerValues, correspondingPlayers);
			if(debug)
				printf("Largest power factor: %.3f\r\n", sortedPowerValues[9] / sortedPowerValues[5]);
			if(!lockoutTimer_running() && (sortedPowerValues[9] > (sortedPowerValues[5] * FUDGE_FACTOR)))
			{
				hitLedTimer_start();
				lockoutTimer_start();
				hitDetected = true;
				hitCounts[correspondingPlayers[9]]++;
				//printf("hit detected\r\n");
			}
		}
	}
}

// Simply returns the boolean flag that was set set by detector().
bool detector_hitDetected()
{
	return hitDetected;
}

// Clear the detected hit once you have accounted for it.
void detector_clearHit()
{
	hitDetected = false;
}

// Get the current hit counts.
void detector_getHitCounts(detector_hitCount_t hitArray[])
{
	for(int i = 0; i < FILTER_IIR_FILTER_COUNT; i++)
	{
		hitArray[i] = hitCounts[i];
	}
}
