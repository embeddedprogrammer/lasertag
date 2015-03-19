#include <stdio.h>
#include <stdlib.h>
#include "filter.h"
#include "math.h"
#include "supportFiles/intervalTimer.h"
#include "supportFiles/utils.h"
#include "testData.h"
#include "histogram.h"
#include "transmitter.h"
#include "supportFiles/interrupts.h"

#define FILTER_IIR_FILTER_COUNT 10
#define FILTER_IIR_WINDOW_LENGTH 2000 //200 ms * 10 samples/ms.
#define TEST_COMPARE_THRESHOLD 1e-5

#define X_QUEUE_SIZE FILTER_FIR_FILTER_LENGTH
#define Y_QUEUE_SIZE FILTER_IIR_FILTER_LENGTH
#define Q_QUEUE_SIZE FILTER_IIR_WINDOW_LENGTH + 1

//Internal functions
void initXQueue();
void initYQueue();
void initZQueues();
//void initPowerValues();
void filter_array_print(double d[FILTER_IIR_FILTER_COUNT], int len);
void filter_array_double_print(double d[FILTER_IIR_FILTER_COUNT][TEST_DATA_COUNT], int row, int len);


queue_t xQueue;
queue_t yQueue;
queue_t zQueue[FILTER_IIR_FILTER_COUNT];

double currentPowerValue[FILTER_IIR_FILTER_COUNT];

bool filter_initFlag = false;

// Init queues and fill them with 0s.
// This must be called before invoking any filter functions.
void filter_init()
{
	if(!filter_initFlag)
	{
		initXQueue();  // Call queue_init() on xQueue and fill it with zeros.
		initYQueue();  // Call queue_init() on yQueue and fill it with zeros.
		initZQueues(); // Call queue_init() on all of the zQueues and fill each z queue with zeros.
		intervalTimer_init(2);
		filter_initFlag = true;
	}
	else
		printf("Filters already initialized\r\n");
}

void initXQueue()
{
	queue_init(&xQueue, X_QUEUE_SIZE);
	queue_fillWithZeros(&xQueue, X_QUEUE_SIZE);
}

void initYQueue()
{
	queue_init(&yQueue, Y_QUEUE_SIZE);
	queue_fillWithZeros(&yQueue, Y_QUEUE_SIZE);
}

void initZQueues()
{
	for(int i = 0; i < FILTER_IIR_FILTER_COUNT; i++)
	{
		queue_init(&zQueue[i], Q_QUEUE_SIZE);
		queue_fillWithZeros(&zQueue[i], Q_QUEUE_SIZE);
	}
}

//void initPowerValues()
//{
//	for(int i = 0; i < FILTER_IIR_FILTER_COUNT; i++)
//	{
//		currentPowerValue[i] = 0;
//	}
//}

// add a new input to the FIR history queue
void filter_addNewInput(double x)
{
	queue_overwritePush(&xQueue, x);
}

void filter_runTestOld()
{
	filter_init();
	for(int i = 0; i < TEST_DATA_COUNT; i++)
	{
		queue_overwritePush(&xQueue, TEST_FIR_INPUT_DATA[i]);
		filter_firFilter();

		if (fabs(queue_readElementFromEnd(&yQueue, 0) - TEST_FIR_OUTPUT_DATA[i]) > TEST_COMPARE_THRESHOLD)
		{
			printf("Calc: %.1e\n", queue_readElementFromEnd(&yQueue, 0));
			printf("Should be: %.1e\n", TEST_FIR_OUTPUT_DATA[i]);
			printf("Difference: %.1e\n", queue_readElementFromEnd(&yQueue, 0) - TEST_FIR_OUTPUT_DATA[i]);
			printf("FIR filter failure! First failure detected at index: %d\n", i);
			return;
		}
	}
	printf("FIR filters passed tests!\n");

	for(int filterNumber = 0; filterNumber < FILTER_IIR_FILTER_COUNT; filterNumber++)
	{
		queue_empty(&yQueue);
		queue_fillWithZeros(&yQueue, FILTER_IIR_FILTER_LENGTH + 1);
		for(int i = 0; i < TEST_DATA_COUNT; i++)
		{
			queue_overwritePush(&yQueue, TEST_IIR_INPUT_DATA[i]);
			filter_iirFilter(filterNumber);
			if (fabs(queue_readElementFromEnd(&zQueue[filterNumber], 0) - TEST_IIR_OUTPUT_DATA[filterNumber][i]) > TEST_COMPARE_THRESHOLD)
			{
				printf("IIR filter %d failure! First failure detected at index: %d\n", filterNumber + 1, i);
				return;
			}
		}
	}

	printf("IIR filters passed tests!\n");

	for(int filterNumber = 0; filterNumber < FILTER_IIR_FILTER_COUNT; filterNumber++)
	{
		queue_empty(&zQueue[filterNumber]);
		queue_fillWithZeros(&zQueue[filterNumber], FILTER_IIR_WINDOW_LENGTH);
		for(int i = 0; i < TEST_DATA_COUNT; i++)
		{
			queue_overwritePush(&zQueue[filterNumber], TEST_IIR_OUTPUT_DATA[filterNumber][i]);

			double power = filter_computePower(filterNumber, (filterNumber == 0), (i == 0));
			//printf("%e\t%e\t%e\r\n", TEST_IIR_OUTPUT_DATA[filterNumber][i], power, TEST_POWER_OUTPUT_DATA[filterNumber][i]);
			if (fabs(power - TEST_POWER_OUTPUT_DATA[filterNumber][i]) > TEST_COMPARE_THRESHOLD)
			{
				printf("Filter %d power computation failure! First failure detected at index: %d\n", filterNumber + 1, i);
				return;
			}
		}
	}

	printf("Power computation passed all tests!\n");
}

void filter_runTestChain()
{
	filter_init();

	for(int i = 0; i < TEST_DATA_COUNT; i++)
	{
		queue_overwritePush(&xQueue, TEST_FIR_INPUT_DATA[i]);
		if(i % FILTER_FIR_DECIMATION_FACTOR == 0)
		{
			filter_firFilter();
			for(int filterNumber = 0; filterNumber < FILTER_IIR_FILTER_COUNT; filterNumber++)
			{
				filter_iirFilter(filterNumber);
				double power = filter_computePower(filterNumber, (filterNumber == 0), (i == 0 && filterNumber < 2));
				if (fabs(power - TEST_POWER_OUTPUT_DATA[filterNumber][i / FILTER_FIR_DECIMATION_FACTOR]) > TEST_COMPARE_THRESHOLD)
				{
					printf("Filter %d power computation failure! First failure detected at index: %d\n", filterNumber + 1, i / 10);
					return;
				}
			}
		}
	}
	printf("Chain passed tests!\n");
}

void filter_array_double_print(double d[FILTER_IIR_FILTER_COUNT][TEST_DATA_COUNT], int row, int len)
{
	for(int i = 0; i < len; i++)
		printf("%.16e%s", d[row][i], (i < len - 1) ? ", " : "");
}

void filter_array_print(double d[FILTER_IIR_FILTER_COUNT], int len)
{
	for(int i = 0; i < len; i++)
		printf("%.16e%s", d[i], (i < len - 1) ? ", " : "");
}

//Applies a decimating FIR filter to the data set using a variation of conv
double filter_firFilter()
{
	double elementSum = 0;

	// Sum x[n]
	for(int i = 0; i < FILTER_FIR_FILTER_LENGTH; i++)
		elementSum += queue_readElementFromEnd(&xQueue, i) * FIR_IMPULSE_RESPONSE[i];
	queue_overwritePush(&yQueue, elementSum);
	//return elementSum;
}

// Uses direct form I
double filter_iirFilter(uint16_t filterNumber)
{
	double elementSum = 0;

	// Sum x[n]
	for(int i = 0; i < FILTER_IIR_FILTER_LENGTH; i++)
		elementSum += queue_readElementFromEnd(&yQueue, i) * IIR_B_COEFFICIENTS[filterNumber][i];

	// Sum y[n]
	for(int i = 1; i < FILTER_IIR_FILTER_LENGTH; i++)
		elementSum -= queue_readElementFromEnd(&zQueue[filterNumber], i - 1) * IIR_A_COEFFICIENTS[filterNumber][i];
//	if(fabs(elementSum) > 1e3) //TODO: Find out why IIR filters are exploding.
//	{
//		elementSum = elementSum / fabs(elementSum);
//	}
//	else if(isnan(elementSum))
//	{
//		elementSum = 0;
//		printf("Warning: NAN in IIR filter %d!\r\n", filterNumber);
//	}
	queue_overwritePush(&zQueue[filterNumber], elementSum);
	//return elementSum;
}

// Use this to compute the power for values contained in a queue.
// If force == true, then recompute everything from scratch.
double filter_computePower(uint16_t filterNumber, bool forceComputeFromScratch, bool debugPrint)
{
	if(debugPrint)
	{
		intervalTimer_reset(2);
		intervalTimer_start(2);
	}
	if(forceComputeFromScratch)
	{
		double sum = 0;
		for(int j = 0; j < FILTER_IIR_WINDOW_LENGTH; j++)
		{
			double element = queue_readElementFromEnd(&zQueue[filterNumber], j);
			sum += element * element;
		}
		currentPowerValue[filterNumber] = sum; // / FILTER_IIR_WINDOW_LENGTH;
	}
	else
	{
//		interrupts_disableArmInts();

		double previousValue = currentPowerValue[filterNumber];

//		if(filterNumber == 0)
//			printf("before: %e ", currentPowerValue[filterNumber]);
//		if(isnan(currentPowerValue[filterNumber]))
//			currentPowerValue[filterNumber] = 0;
		double elementIn = 1; //queue_readElementFromEnd(&zQueue[filterNumber], 0); TODO: actually do stuff.
		double elementOut = 1; //queue_readElementFromEnd(&zQueue[filterNumber], FILTER_IIR_WINDOW_LENGTH);
		currentPowerValue[filterNumber] += 1; //(elementIn * elementIn);
		currentPowerValue[filterNumber] -= 1; //(elementOut * elementOut);
//		for(volatile int i = 0; i < 1000; i++);

		if(fabs(previousValue - currentPowerValue[filterNumber]) > 1)
			printf("before %e after: %e transmitter state: %d\r\n", previousValue,
					currentPowerValue[filterNumber], transmitter_getState());
//			printf("before %e after: %e\r\n", previousValue,
//					currentPowerValue[filterNumber]);

//		if(filterNumber == 0)
//			printf("after: %e\r\n", currentPowerValue[filterNumber]);

//		interrupts_enableArmInts();
	}
//	if(debugPrint)
//	{
//	  double runningSeconds;
//	  intervalTimer_stop(2);
//	  intervalTimer_getTotalDurationInSeconds(2, &runningSeconds);
//	  printf("Time to compute power %s: %e\n\r", forceComputeFromScratch ? "from scratch" : "with reused values", runningSeconds);
//	}
	return currentPowerValue[filterNumber];
}

double filter_getCurrentPowerValue(uint16_t filterNumber)
{
	return currentPowerValue[filterNumber];
}

// Uses the last computed-power values, scales them to the provided lowerBound and upperBound args, returns the index of element containing the max value.
// The caller provides the normalizedArray that will contain the normalized values. indexOfMaxValue indicates the channel with max. power.
void filter_getNormalizedPowerValues(double normalizedArray[], uint16_t* indexOfMaxValue)
{
	float maxPower = 0;
	for(int i = 0; i < FILTER_IIR_FILTER_COUNT; i++)
		if(currentPowerValue[i] > maxPower)
			maxPower = currentPowerValue[i];
	for(int i = 0; i < FILTER_IIR_FILTER_COUNT; i++)
	{
		normalizedArray[i] = currentPowerValue[i] / maxPower;
	}
}

void filter_getSortedPowerValues(double sortedPowerValues[], int correspondingPlayers[])
{
	bool used[10] = {false};
	for(int i = 0; i < FILTER_IIR_FILTER_COUNT; i++)
	{
		double minPower = 1e10;
		int minFilter = 0;
		for(int j = 0; j < FILTER_IIR_FILTER_COUNT; j++)
		{
			if(!used[j] && currentPowerValue[j] < minPower)
			{
				minPower = currentPowerValue[j];
				minFilter = j;
			}
		}
		used[minFilter] = true;
		sortedPowerValues[i] = minPower;
		correspondingPlayers[i] = minFilter;
		//printf("power[%d] = %f, player[%d] = %d\r\n", i, minPower, i, minFilter);
	}
}

void filter_printQueues()
{
	printf("ADC data queue: ");
	queue_print(&xQueue);
	printf("FIR results: ");
	queue_print(&yQueue);
	printf("IIR filter results: ");
	for(int filterNumber = 0; filterNumber < FILTER_IIR_FILTER_COUNT; filterNumber++)
	{
		queue_print(&zQueue[filterNumber]);
	}

	printf("ADC data queue: ");
	queue_printMaxValue(&xQueue);
	printf("FIR results: ");
	queue_printMaxValue(&yQueue);
	printf("IIR filter results: ");
	for(int filterNumber = 0; filterNumber < FILTER_IIR_FILTER_COUNT; filterNumber++)
	{
		queue_printMaxValue(&zQueue[filterNumber]);
	}
}

/*=============================================================================
 * ================= Test Routines Start Here =================================
 ==============================================================================*/

/* ===================== Test Variable Definitions ==================== */
static uint16_t firDecimationCount = 0; // Used to keep track of samples relative to the decimation factor.

/* ================================== #defines ================================ */
#define FILTER_IIR_POWER_TEST_PERIOD_COUNT FILTER_FREQUENCY_COUNT
#define FILTER_FREQUENCY_COUNT FILTER_IIR_FILTER_COUNT
#define FILTER_SAMPLE_FREQUENCY_IN_KHZ 100

// NOTE: YOU MUST DEFINE THIS PROPERLY FOR THE TEST TO WORK PROPERLY.
// If you used a leading 1 in the A-coefficient array for your IIR, this should be defined to be a 1.
// If you did not use a leading 1 in the A-coefficient array for your IIR, this should be defined to be 0.
#define FILTER_TEST_USED_LEADING_1_IN_IIR_A_COEFFICIENT_ARRAY 1

/* ===================== Simple Accessor Functions ==================== */

// Returns the array of FIR coefficients.
double* filterTest_getFirCoefficientArray()
{
	return FIR_IMPULSE_RESPONSE;
}

// Returns the number of FIR coefficients.
uint32_t filterTest_getFirCoefficientCount()
{
	return FILTER_FIR_FILTER_LENGTH;
}

// Returns the array of coefficients for a particular filter number.
double* filterTest_getIirACoefficientArray(uint16_t filterNumber)
{
	return IIR_A_COEFFICIENTS[filterNumber];
}

// Returns the number of A coefficients.
uint32_t filterTest_getIirACoefficientCount()
{
	return FILTER_IIR_FILTER_LENGTH;
}

// Returns the array of coefficients for a particular filter number.
double* filterTest_getIirBCoefficientArray(uint16_t filterNumber)
{
	return IIR_B_COEFFICIENTS[filterNumber];
}

// Returns the number of B coefficients.
uint32_t filterTest_getIirBCoefficientCount()
{
	return FILTER_IIR_FILTER_LENGTH;
}

// Returns the size of the yQueue.
uint32_t filterTest_getYQueueSize()
{
	return Y_QUEUE_SIZE;
}

// Returns the decimation value.
uint16_t filterTest_getDecimationValue()
{
	return FILTER_FIR_DECIMATION_FACTOR;
}

// Returns the starting index for the A-coefficient array for the IIR-filter.
// Some students use the leading 1, others did not.
uint16_t filterTest_getIirACoefficientArrayStartingIndex()
{
	if (FILTER_TEST_USED_LEADING_1_IN_IIR_A_COEFFICIENT_ARRAY)
		return 1;
	else
		return 0;
}

/*==========================Helper Test Functions ============================ */

// Returns true if the values are within TEST_FILTER_FLOATING_POINT_EPSILON of each other.
//#define TEST_FILTER_FLOATING_POINT_EPSILON 1.0E-15L
static const double eps = 1.0E-15;
bool filterTest_floatingPointEqual(double a, double b)
{
	//	return fabs(a-b) < TEST_FILTER_FLOATING_POINT_EPSILON;
	return fabs(a - b) < eps;
}

// Fills the queue with the fillValue, overwriting all previous contents.
void filterTest_fillQueue(queue_t* q, double fillValue)
{
	for (queue_size_t i = 0; i < q->size; i++)
	{
		queue_overwritePush(q, fillValue);
	}
}

queue_data_t filterTest_filter_readMostRecentValueFromQueue(queue_t* q)
{
	return queue_readElementAt(q, q->elementCount - 1);
}

/* ============================ Major Test Functions ============================= */

// Invokes FIR-filter according to the decimation factor.
// Returns true if it invokes filter_firFilter().
bool filter_decimatingFirFilter()
{
	if (firDecimationCount == filterTest_getDecimationValue() - 1)
	{ // Time to run the FIR filter?
		filter_firFilter();      // Run the FIR filter.
		firDecimationCount = 0;  // Reset the decimation count.
		return true;             // Return true because you ran the FIR filter.
	}
	firDecimationCount++; // Increment the decimation count each time you call this filter.
	return false;          // Didn't run the FIR filter.
}

// Pushes a single 1.0 through the xQueue. Golden output data are just the FIR coefficients in reverse order.
// If this test passes, you are multiplying the coefficient with the correct element of xQueue.
bool filterTest_runFirAlignmentTest(bool printMessageFlag)
{
	bool success = true;	 					// Be optimistic.
	filterTest_fillQueue(&xQueue, 0.0);	// zero-out the xQueue.
	filter_addNewInput(1.0);				// Place a single 1.0 in the xQueue.
	for (uint32_t i = 0; i < filterTest_getFirCoefficientCount(); i++)
	{				// Push the single 1.0 through the queue.
		filter_firFilter();								// Run the FIR filter.
		double firValue = filterTest_filter_readMostRecentValueFromQueue(
				&yQueue);
		double firGoldenOutput =
				filterTest_getFirCoefficientArray()[filterTest_getFirCoefficientCount()
						- i - 1];// Golden output is simply the FIR coefficient.
		if (!filterTest_floatingPointEqual(firValue, firGoldenOutput))
		{// If the output from the FIR filter does not match the golden value, print an error.
			success = false;								// The test failed.
			printf(
					"filter_runAlignmentTest: Output from FIR Filter(%le) does not match test-data(%le).\n\r",
					firValue, firGoldenOutput);
		}
		filter_addNewInput(0.0);// Shift the 1.0 value over one position in the queue.
	}
	// Print informational messages.
	if (printMessageFlag)
	{
		printf("filter_runFirAlignmentTest ");
		if (success)
			printf("passed.\n\r");
		else
			printf("failed.\n\r");
	}
	return success;	// Return the success of failure of this test.
}

// Fills the xQueue with 1.0. Invokes filter_firFilter() and compares the output with the
// sum of the coefficients.
bool filterTest_runFirArithmeticTest(bool printMessageFlag)
{
	bool success = true;							// Be optimistic.
	filterTest_fillQueue(&xQueue, 0.0);  // zero-out the xQueue.
	double firGoldenOutput = 0.0; // You will compute the golden output by accumulating the FIR coefficients.
	for (uint32_t i = 0; i < filterTest_getFirCoefficientCount(); i++)
	{  // Loop enough times to go through the coefficients.
		firGoldenOutput += filterTest_getFirCoefficientArray()[i];
	}
	filterTest_fillQueue(&xQueue, 1.0);
	filter_firFilter();
	double firValue = filterTest_filter_readMostRecentValueFromQueue(&yQueue);
	if (!filterTest_floatingPointEqual(firValue, firGoldenOutput))
	{
		success = false;									// Test failed.
		printf(
				"filter_runArithmeticTest: Output from FIR Filter(%le) does not match test-data(%le).\n\r",
				firValue, firGoldenOutput);
	}
	// Print informational messages.
	if (printMessageFlag)
	{
		printf("filter_runFirArithmeticTest ");
		if (success)
			printf("passed.\n\r");
		else
			printf("failed.\n\r");
	}
	return success;	// Return the success or failure of the test.
}

// This test checks to see that the B coefficients are multiplied with the correct values of the yQueue.
// If it passes, the coefficients are properly aligned with the data in yQueue.
// This test only checks the coefficients for filterNumber (frequency).
bool filterTest_runIirBAlignmentTest(uint16_t filterNumber,
		bool printMessageFlag)
{
	bool success = true;						// Be optimistic.
	filterTest_fillQueue(&yQueue, 0.0);	// zero-out the yQueue.
	filterTest_fillQueue(&(zQueue[filterNumber]), 0.0);	// zero out the zQueue for filterNumber.
	queue_overwritePush(&yQueue, 1.0);		// Place a single 1.0 in the yQueue.
	for (uint32_t i = 0; i < filterTest_getIirBCoefficientCount(); i++)
	{
		filter_iirFilter(filterNumber);					// Run the IIR filter.
		double iirValue = filterTest_filter_readMostRecentValueFromQueue(
				&(zQueue[filterNumber]));				// Run the IIR filter.
		double iirGoldenOutput = filterTest_getIirBCoefficientArray(
				filterNumber)[i];	// Golden output is simply the coefficient.
		if (!filterTest_floatingPointEqual(iirValue, iirGoldenOutput))
		{	// Make sure they IIR output matches the golden output.
			success = false;			// Note test failure and print message.
			printf(
					"filter_runIirBlignmentTest: Output from IIR Filter[%d](%le) does not match test-data(%le) at index(%ld).\n\r",
					filterNumber, iirValue, iirGoldenOutput, i);
		}
		filterTest_fillQueue(&(zQueue[filterNumber]), 0.0);	// zero out the zQueue for filterNumber so the A-summation is always 0.
		queue_overwritePush(&yQueue, 0.0);// Shift the 1.0 over one position in the yQueue.
	}
	// Print informational messages.
	if (printMessageFlag)
	{
		printf("filter_runIirBAlignmentTest ");
		if (success)
			printf("passed.\n\r");
		else
			printf("failed.\n\r");
	}
	return success;	// Return the success or failure of the test.
}

// Checks to see that the A-coefficients are properly aligned with the zQueue.
// This test checks the A-coefficients for a specific filter-number (frequency).
// It is a little more work to create a sliding 1 in the zQueue because the IIR updates the zQueue.
// Thus as you move across coefficients, the zQueue is cleared out. 1.0 is added and shifted over the correct number of spaces.
bool filterTest_runIirAAlignmentTest(uint16_t filterNumber,
		bool printMessageFlag)
{
	bool success = true;						// Be optimistic.
	filterTest_fillQueue(&yQueue, 0.0);	// zero-out the yQueue so the B-summation is always 0.
	uint16_t startingIndex = filterTest_getIirACoefficientArrayStartingIndex(); // Varies according to student.
	for (uint32_t i = 0;
			i < filterTest_getIirACoefficientCount() - startingIndex; i++)
	{ // Loop through all of the A-coefficients.
		filterTest_fillQueue(&(zQueue[filterNumber]), 0.0);	// zero out the zQueue for filterNumber.
		queue_overwritePush(&(zQueue[filterNumber]), 1.0);// Add a single 1.0 to the queue.
		for (uint32_t j = 0; j < i; j++)
		{// Move over the 1.0 an additional position each time through the loop.
			queue_overwritePush(&(zQueue[filterNumber]), 0.0);// Move the 1.0 over by writing the correct number of zeros.
		}
		filter_iirFilter(filterNumber);					// Run the IIR filter.
		double iirValue = filterTest_filter_readMostRecentValueFromQueue(
				&(zQueue[filterNumber]));				// Run the IIR filter.
		double iirGoldenOutput = filterTest_getIirACoefficientArray(
				filterNumber)[i + startingIndex];// Golden output is simply the coefficient.
		if (!filterTest_floatingPointEqual(iirValue, -iirGoldenOutput))
		{// Check to see that the IIR-output matches the correct coefficient value.
			success = false;// Note the failure of the test and print an info message.
			printf(
					"filter_runIirAlignmentTest: Output from IIR Filter[%d](%le) does not match test-data(%le) at index(%ld).\n\r",
					filterNumber, iirValue, iirGoldenOutput, i);
		}
	}
	// Print informational messages.
	if (printMessageFlag)
	{
		printf("filter_runIirAAlignmentTest ");
		if (success)
			printf("passed.\n\r");
		else
			printf("failed.\n\r");
	}
	return success;	// Return the failure or success of the test.
}

// Normalizes the values in the array argument.
void filterTest_normalizeArrayValues(double* array, uint16_t size)
{
	// Find the maximum value
	uint16_t maxIndex = 0;
	// Find the index of the max value in the array.
	for (int i = 0; i < size; i++)
	{
		if (array[i] > array[maxIndex])
			maxIndex = i;
	}
	double maxPowerValue = array[maxIndex];
	// Normalize everything between 0.0 and 1.0, based upon the max value.
	for (int i = 0; i < size; i++)
		array[i] = array[i] / maxPowerValue;
}

#define FILTER_FIR_POWER_TEST_PERIOD_COUNT 22
#define FILTER_TEST_HISTOGRAM_BAR_COUNT FILTER_FIR_POWER_TEST_PERIOD_COUNT
// Everything is defined assuming a 100 kHz sample rate.
static const uint16_t filter_testPeriodTickCounts[FILTER_FIR_POWER_TEST_PERIOD_COUNT] =
		{ 90, 72, 58, 50, 44, 38, 34, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12,
				10, 8, 6, 4, 2 };
#define MAX_BUF 10
#define FILTER_TEST_PULSE_WIDTH_LENGTH 20000

// Plots the frequency response of the FIR filter on the TFT.
// Everything is defined assuming a 100 kHz sample rate.
// Frequencies run from 1.1 kHz to 50 kHz.
void filterTest_runFirPowerTest(bool printMessageFlag)
{
	if (printMessageFlag)
	{
		// Tells you that this function is plotting the frequency response for the FIR filter for a set of frequencies.
		printf(
				"running filter_runFirPowerTest() - plotting power values (frequency response) for frequencies %1.2lf kHz to %1.2lf kHz for FIR filter to TFT display.\n\r",
				((double) ((FILTER_SAMPLE_FREQUENCY_IN_KHZ))
						/ filter_testPeriodTickCounts[0]),
				((double) ((FILTER_SAMPLE_FREQUENCY_IN_KHZ))
						/ filter_testPeriodTickCounts[FILTER_FIR_POWER_TEST_PERIOD_COUNT
								- 1]));
	}
	firDecimationCount = 0;
	double testPeriodPowerValue[FILTER_FIR_POWER_TEST_PERIOD_COUNT];// Computed power values will go here.
	uint16_t freqCount = 0;	// Used to print info message.
	// Simulate running everything at 100 kHz. Simply add either 1.0 or -1.0 to xQueue based upon the
	// the frequency you are simulating.
	// Iterate over all of the test-periods.
	for (uint16_t testPeriodIndex = 0;
			testPeriodIndex < FILTER_FIR_POWER_TEST_PERIOD_COUNT;
			testPeriodIndex++)
	{
		uint16_t currentPeriodTickCount =
				filter_testPeriodTickCounts[testPeriodIndex];
		double power = 0.0;					// Temp variable to compute power.
		uint32_t totalTickCount = 0;// Keep track of where you are in the period.
		while (totalTickCount < FILTER_TEST_PULSE_WIDTH_LENGTH)
		{		// Keep going until you have completed the entire pulse-width.
			for (uint16_t freqTick = 0; freqTick < currentPeriodTickCount;
					freqTick++)
			{	// This loop completes a single period.
				if (freqTick < currentPeriodTickCount / 2)// The first half of the period is -1.0.
					filter_addNewInput(-1.0);// Add the simulated value to the FIR filter.
				else
					filter_addNewInput(+1.0);// In the second half of the period, add +1.0 to the FIR filter.
				if (filter_decimatingFirFilter())
				{// Tells us if we ran the the FIR filter (we are decimating so the FIR filter only runs every 10 input values).
					double firOutput =
							filterTest_filter_readMostRecentValueFromQueue(
									&yQueue);// Get the output from the yQueue.
					power += firOutput * firOutput;	// Compute the power so far.
					totalTickCount++;
				}
			}
		}
		testPeriodPowerValue[testPeriodIndex] = power;// All done computing power for this period, move on to the next one.
		//		printf("Frequency[%d](%5.4lf) Power:%5.2le\n\r", freqCount, (double) (FILTER_SAMPLE_FREQUENCY_IN_KHZ/currentPeriodTickCount), power);
		freqCount++;
	}
	// Now we start plotting the results.
	filterTest_normalizeArrayValues(testPeriodPowerValue,
			FILTER_FIR_POWER_TEST_PERIOD_COUNT);	// Normalize the values.
	histogram_init(FILTER_TEST_HISTOGRAM_BAR_COUNT);	// Init the histogram.
	// Set labels and colors (blue) for the standard frequencies 0-9.
	for (int i = 0; i < FILTER_FREQUENCY_COUNT; i++)
	{
		histogram_setBarColor(i, DISPLAY_BLUE);	// Sets the color of the bar.
		char tempLabel[MAX_BUF];		// Temp variable for label generation.
		snprintf(tempLabel, MAX_BUF, "%d", i);// Create the label that represents one of the user frequencies.
		histogram_setBarLabel(i, tempLabel);		// Finally, set the label.
	}
	// Set the colors for the other nonstandard frequencies to be red so that the stand out.
	for (int i = 10; i < FILTER_FIR_POWER_TEST_PERIOD_COUNT; i++)
	{
		histogram_setBarColor(i, DISPLAY_RED);
		char tempLabel[MAX_BUF];	// Used to create labels.
		// Create three kinds of labels.
		// 1. Just label the first set of defined frequencies 0-9.
		// 2. This is the start of the frequencies outside the actual transmitted frequencies.
		// The bounds are printed at the start and end of this range, using the labels so that they display OK in the limited space.
		// 3. The lower bound for out-of-bound testing frequencies.
		if (i == 10)
		{
			snprintf(tempLabel, MAX_BUF, "%dkHz",
					FILTER_SAMPLE_FREQUENCY_IN_KHZ
							/ filter_testPeriodTickCounts[i]);
			histogram_setBarLabel(i, tempLabel);
			// 4. Print the upper bound for out-of-bound testing frequencies further to the left to make readable.
		}
		else if (i == FILTER_FIR_POWER_TEST_PERIOD_COUNT - 4)
		{
			snprintf(tempLabel, MAX_BUF, "%dkHz",
					FILTER_SAMPLE_FREQUENCY_IN_KHZ
							/ filter_testPeriodTickCounts[FILTER_FIR_POWER_TEST_PERIOD_COUNT
									- 1]);
			histogram_setBarLabel(i, tempLabel);
			// Print a "-" for readability.
		}
		else if (i == FILTER_FIR_POWER_TEST_PERIOD_COUNT - 7)
		{
			histogram_setBarLabel(i, "-");
			// Print blank space everywhere else.
		}
		else
		{
			histogram_setBarLabel(i, "");

		}
	}
	// Set the actual histogram-bar data to be power values..
	for (uint16_t barIndex = 0; barIndex < FILTER_TEST_HISTOGRAM_BAR_COUNT;
			barIndex++)
	{
		histogram_setBarData(barIndex,
				testPeriodPowerValue[barIndex]
						* HISTOGRAM_MAX_BAR_DATA_IN_PIXELS, "");
	}
	histogram_redrawBottomLabels(); // Need to redraw the bottom labels because I changed the colors.
	histogram_updateDisplay();	   // Redraw the histogram.
}

// Plots frequency response for the selected filterNumber against the 10 standard frequencies.
void filterTest_runIirPowerTest(uint16_t filterNumber, bool printMessageFlag)
{
	if (printMessageFlag)
	{
		printf("running filter_runFirPowerTest(%d) - plotting power for all player frequencies for IIR filter(%d) to TFT display.\n\r",	filterNumber, filterNumber);
	}
	firDecimationCount = 0;
	double testPeriodPowerValue[FILTER_IIR_POWER_TEST_PERIOD_COUNT];// Keep track of power values here.
	uint16_t freqCount = 0;
	// Simulate running everything at 100 kHz.
	for (uint16_t testPeriodIndex = 0; testPeriodIndex < FILTER_FREQUENCY_COUNT; testPeriodIndex++)
	{  // Only use the first 10 standard frequencies.
		uint16_t currentPeriodTickCount =
				filter_testPeriodTickCounts[testPeriodIndex]; // You will be generating a frequency with this period.
		double power = 0.0;						// Used to compute power.
		uint32_t totalTickCount = 0;// Keep track of where you are in the given period.
		while (totalTickCount < FILTER_TEST_PULSE_WIDTH_LENGTH) // Generate a pulse-width of periods.
		{
			double iirOutput = 0.0;		// Output from the IIR filter.
			for (uint16_t freqTick = 0; freqTick < currentPeriodTickCount;
					freqTick++)
			{		// This loop generates one period of the frequency.
				if (freqTick < currentPeriodTickCount / 2)// First half of period is -1.0.
					filter_addNewInput(-1.0);
				else
					filter_addNewInput(+1.0);	// Second half of period is +1.0
				if (filter_decimatingFirFilter())
				{	// If you actually ran the FIR-filter, run the IIR-filter.
					filter_iirFilter(filterNumber);
					iirOutput = filterTest_filter_readMostRecentValueFromQueue(&(zQueue[filterNumber]));
				}
				power += iirOutput * iirOutput;	// Compute the power at the output.
				totalTickCount++;
			}
		}
		testPeriodPowerValue[testPeriodIndex] = power;
		//		printf("Frequency[%d](%5.4lf) Power:%5.2le\n\r", freqCount, (double) (FILTER_SAMPLE_FREQUENCY_IN_KHZ/currentPeriodTickCount), power);
		freqCount++;
	}
	// Make a copy of the power values that you will normalize.
	double normalizedPowerValue[FILTER_IIR_POWER_TEST_PERIOD_COUNT];
	for (int i = 0; i < FILTER_IIR_POWER_TEST_PERIOD_COUNT; i++)
	{
		normalizedPowerValue[i] = testPeriodPowerValue[i];
	}
	filterTest_normalizeArrayValues(normalizedPowerValue,
			FILTER_IIR_POWER_TEST_PERIOD_COUNT);
	histogram_init(FILTER_IIR_POWER_TEST_PERIOD_COUNT);
	// Set labels and colors (red) for the standard frequencies 0-9.
	// Default is red but will change the color for the desired frequency to be blue.
	for (int i = 0; i < FILTER_IIR_POWER_TEST_PERIOD_COUNT; i++)
	{
		histogram_setBarColor(i, DISPLAY_RED);
		char tempLabel[MAX_BUF];
		snprintf(tempLabel, MAX_BUF, "%d", i);
		histogram_setBarLabel(i, tempLabel);
	}
	histogram_setBarColor(filterNumber, DISPLAY_BLUE);// Desired frequency drawn in blue.
	for (uint16_t barIndex = 0; barIndex < FILTER_IIR_POWER_TEST_PERIOD_COUNT;
			barIndex++)
	{
		char label[HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS];	// Get a buffer for the label.
		// Create the top-label, based upon the actual power value.
		if (snprintf(label, HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS, "%0.0e",
				testPeriodPowerValue[barIndex]) == -1)
			printf(
					"Error: snprintf encountered an error during conversion.\n\r");
		// Pull out the 'e' from the exponent to make better use of your characters.
		trimLabel(label);
		histogram_setBarData(barIndex,
				normalizedPowerValue[barIndex]
						* HISTOGRAM_MAX_BAR_DATA_IN_PIXELS, label);	// No top-label.
	}
	histogram_redrawBottomLabels(); // Need to redraw the bottom labels because I changed the colors.
	histogram_updateDisplay();	   // Redraw the histogram.
}

// 1. Tests the FIR filter in isolation using test-data, with no decimation.
// 2. Tests the FIR-filter using actual data, with decimation, using data generated by the transmitter code but not going
// through the ADC. The data generated by the transmitter are scaled between -1.0 and 1.0 and placed directly in the xQueue.
// The power in the FIR output is measured and compared to a threshold.
// 3. Tests the IIR in isolation using test-data.
// 4. Tests the
#define TEN_SECONDS 10000
#define TWO_SECONDS 2000
bool filter_runTest()
{
	bool success = true;	// Be optimistic.
	filter_init();
	success &= filterTest_runFirAlignmentTest(true);
	success &= filterTest_runFirArithmeticTest(true);
	success &= filterTest_runIirAAlignmentTest(0, true);
	success &= filterTest_runIirBAlignmentTest(0, true);
	filterTest_runFirPowerTest(true);
	utils_msDelay(TEN_SECONDS);
	for (int i = 0; i < FILTER_IIR_FILTER_COUNT; i++)
	{
		filterTest_runIirPowerTest(i, true);
		utils_msDelay(TWO_SECONDS);
	}
	return success;
}



