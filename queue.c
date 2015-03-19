/*
 * queue.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "queue.h"

//Internal function declarations
int queue_runTest();
void queue_incIndexIn(queue_t* q);
void queue_incIndexOut(queue_t* q);
queue_index_t queue_incIndex(queue_index_t index, queue_index_t amt, queue_size_t size);

// Standard queue implementation
void queue_init(queue_t* q, queue_size_t size)
{
	q->indexIn = 0;
	q->indexOut = 0;
	q->elementCount = 0;
	q->size = size;
	q->data = (queue_data_t *) malloc(q->size * sizeof(queue_data_t));
}

// Fill queue with zeros
void queue_fillWithZeros(queue_t* q, int zeroCount)
{
	for(int i = 0; i < zeroCount; i++)
		queue_overwritePush(q, 0);
}

// Returns the size of the queue..
queue_size_t queue_size(queue_t* q)
{
	return q->size;
}

// Returns true if the queue is full.
bool queue_full(queue_t* q)
{
	return (q->elementCount == q->size);
}

// Returns true if the queue is empty.
bool queue_empty(queue_t* q)
{
	return (q->elementCount == 0);
}

// Empty everything in queue
void queue_emptyElements(queue_t* q)
{
	q->indexIn = 0;
	q->indexOut = 0;
	q->elementCount = 0;
}

// Pushes a new element into the queue. Reports an error if the queue is full.
void queue_push(queue_t* q, queue_data_t value)
{
	if(queue_full(q))
	{
		printf("Error! Queue is full\n");
		return;
	}
	q->data[q->indexIn] = value;
	queue_incIndexIn(q);
	q->elementCount++;
}

// Removes the oldest element in the queue.
queue_data_t queue_pop(queue_t* q)
{
	if(queue_empty(q))
	{
		printf("Error! Queue is empty");
		return 0;
	}
	queue_data_t data = q->data[q->indexOut];
	queue_incIndexOut(q);
	q->elementCount--;
	return data;
}

// Pushes a new element into the queue, making room by removing the oldest element.
void queue_overwritePush(queue_t* q, queue_data_t value)
{
	q->data[q->indexIn] = value;
	queue_incIndexIn(q);
	if(queue_full(q))
		queue_incIndexOut(q);
	else
		q->elementCount++;
}

// Provides random-access read capability to the queue.
// Low-valued indexes access older queue elements while higher-value indexes access newer elements
// (according to the order that they were added).
queue_data_t queue_readElementAt(queue_t* q, queue_index_t index)
{
	if(index > queue_elementCount(q) || index < 0)
	{
		printf("Queue access out of bounds: %d", index);
		return 0;
	}
	queue_index_t actualIndex = queue_incIndex(q->indexOut, index, q->size);
	return q->data[actualIndex];
}

queue_data_t queue_readElementFromEnd(queue_t* q, queue_index_t indexFromEnd)
{
	//printf("read at %d\n", queue_elementCount(q) - indexFromEnd - 1);
	return queue_readElementAt(q, queue_elementCount(q) - indexFromEnd - 1);
}

// Returns a count of the elements currently contained in the queue.
queue_size_t queue_elementCount(queue_t* q)
{
	return q->elementCount;
}

// Just free the malloc'd storage.
void gueue_garbageCollect(queue_t* q)
{
	free(q->data);

}

// Prints the current contents of the queue. Handy for debugging.
void queue_print(queue_t* q)
{
	//printf("Queue contents: ");

	for(int i = 0; i < queue_elementCount(q); i++)
		printf("%.1e%s", queue_readElementAt(q, i), (i < queue_elementCount(q) - 1) ? " " : "\n");
		//printf("%f%s", queue_readElementAt(q, i), (i < queue_elementCount(q) - 1) ? ", " : "\n");
	if(queue_elementCount(q) == 0)
		printf("empty\n");
}

// Prints the current contents of the queue. Handy for debugging.
void queue_printMaxValue(queue_t* q)
{
	//printf("Queue contents: ");

	double maxValue = 0;
	for(int i = 0; i < queue_elementCount(q); i++)
	{
		double value = queue_readElementAt(q, i);
		if(value > maxValue || isnan(value))
			maxValue = value;
	}
	if(queue_elementCount(q) == 0)
		printf("empty\r\n");
	else
		printf("Max value: %.2e\r\n", maxValue);
}

// Performs a comprehensive test of all queue functions. Returns 1 if succeeds.
int queue_runTest()
{
	queue_t q;
	queue_size_t size = 10;

	// Test initialization
	printf("Init queue\n");
	queue_init(&q, size);
	queue_print(&q);
	printf("Pop when empty\n");
	queue_pop(&q);
	queue_print(&q);
	if(!queue_empty(&q))
	{
		printf("Queue not empty after init\n");
		return 0;
	}
	if(queue_full(&q))
	{
		printf("Queue is full after init\n");
		return 0;
	}
	if(queue_elementCount(&q) != 0)
	{
		printf("Queue doesn't contain 0 elements\n");
		return 0;
	}
	if(queue_size(&q) != size)
	{
		printf("Queue isn't the correct size\n");
		return 0;
	}

	// Add and remove element
	printf("Add and remove element\n");
	queue_data_t val = 5;
	queue_push(&q, val);
	queue_print(&q);
	if(queue_empty(&q))
	{
		printf("Queue empty after adding first element\n");
		return 0;
	}
	if(queue_full(&q))
	{
		printf("Queue full after adding first element\n");
		return 0;
	}
	queue_pop(&q);
	queue_print(&q);
	if(!queue_empty(&q))
	{
		printf("Queue not empty after removing element\n");
		return 0;
	}

	// Test FIFO property of queue
	printf("Test FIFO property of queue. Push 3 values\n");
	queue_data_t val1 = 7;
	queue_data_t val2 = 9;
	queue_data_t val3 = 2;
	queue_print(&q);
	queue_push(&q, val1);
	queue_print(&q);
	queue_push(&q, val2);
	queue_print(&q);
	queue_push(&q, val3);
	queue_print(&q);
//	if(queue_pop(&q) != val1 || queue_pop(&q) != val2 || queue_pop(&q) != val3)
	printf("Pop 3 values\n");
	if(queue_pop(&q) != val1)
	{
		queue_print(&q);
		if(queue_pop(&q) != val2)
		{
			queue_print(&q);
			if(queue_pop(&q) != val3)
			{
				queue_print(&q);
				printf("Queue failed FIFO test\n");
				return 0;
			}
		}
	}

	// Test overwrite feature
	printf("Push and fail\n");
	for(int i = 0; i < size + 3; i++)
	{
		printf("push %f\n", i);
		queue_push(&q, i);
		queue_print(&q);
	}
	printf("Overwrite push\n");
	for(int i = 0; i < size + 3; i++)
	{
		printf("push %f\n", i);
		queue_overwritePush(&q, i);
		queue_print(&q);
	}
	if(queue_elementCount(&q) != size ||
			queue_readElementAt(&q, 0) != 3 ||
			queue_readElementAt(&q, size - 1) != size + 2)
	{
		printf("Queue failed overwrite test\n");
		return 0;
	}

	// Test for breaks in circular array of queue
	printf("Test for breaks in circular array of queue\n");
	for(int i = size + 3; i < size * 3; i++)
	{
		if(queue_pop(&q) != i - size)
		{
			printf("Queue failed circular array test\n");
			return 0;
		}
		queue_push(&q, i);
		queue_print(&q);
	}
	printf("Passed all tests\n");

	//Passed all tests
	return 1;
}

// Increments the in index
void queue_incIndexIn(queue_t* q)
{
	q->indexIn = queue_incIndex(q->indexIn, 1, q->size);
}

// Increments the in index
void queue_incIndexOut(queue_t* q)
{
	q->indexOut = queue_incIndex(q->indexOut, 1, q->size);
}

// Increments index. Rolls over when reaches size.
queue_index_t queue_incIndex(queue_index_t index, queue_index_t amt, queue_size_t size)
{
	index += amt;
	if(index >= size)
		index -= size;
	return index;
}

#define SMALL_QUEUE_SIZE 10
#define SMALL_QUEUE_COUNT 10
static queue_t smallQueue[SMALL_QUEUE_COUNT];
static queue_t largeQueue;

// smallQueue[SMALL_QUEUE_COUNT-1] contains newest value, smallQueue[0] contains oldest value.
// Thus smallQueue[0](0) contains oldest value. smallQueue[SMALL_QUEUE_COUNT-1](SMALL_QUEUE_SIZE-1) contains newest value.
// Presumes all queue come initialized full of something (probably zeros).
static double popAndPushFromChainOfSmallQueues(double input) {
  // Grab the oldest value from the oldest small queue before it is "pushed" off.
  double willBePoppedValue = queue_readElementAt(&(smallQueue[0]), 0);
  // Sequentially pop from the next newest queue and push into next oldest queue.
  for (int i=0; i<SMALL_QUEUE_COUNT-1; i++) {
    queue_overwritePush(&(smallQueue[i]), queue_pop(&(smallQueue[i+1])));
  }
  queue_overwritePush(&(smallQueue[SMALL_QUEUE_COUNT-1]), input);
  return willBePoppedValue;
}

static bool compareChainOfSmallQueuesWithLargeQueue(uint16_t iterationCount) {
  bool success = true;
  static uint16_t oldIterationCount;
  static bool firstPass = true;
  // Start comparing the oldest element in the chain of small queues, and the large queue
  // and move towards the newest values.
  for (uint16_t smallQIdx=0; smallQIdx<SMALL_QUEUE_COUNT; smallQIdx++) {
    for (uint16_t smallQEltIdx=0; smallQEltIdx<SMALL_QUEUE_SIZE; smallQEltIdx++) {
      double smallQElt = queue_readElementAt(&(smallQueue[smallQIdx]), smallQEltIdx);
      double largeQElt = queue_readElementAt(&largeQueue, (smallQIdx*SMALL_QUEUE_SIZE) + smallQEltIdx);
      if (smallQElt != largeQElt) {
	if (firstPass || (iterationCount != oldIterationCount)) {
	  printf("Iteration:%d\n", iterationCount);
	  oldIterationCount = iterationCount;
	  firstPass = false;
	}
	printf("largeQ(%d):%lf", (smallQIdx*SMALL_QUEUE_SIZE) + smallQEltIdx, largeQElt);
	printf(" != ");
	printf("smallQ[%d](%d): %lf\n", smallQIdx, smallQEltIdx, smallQElt);
        success = false;
      }
    }
  }
  return success;
}

#define TEST_ITERATION_COUNT 105
#define FILLER 5
int queue_runTest2() {
  int success = 1;  // Be optimistic.
  // Let's make this a real torture test by testing queues against themselves.
  // Test the queue against an array to make sure there is agreement between the two.
  double testData[SMALL_QUEUE_SIZE + FILLER];
  queue_t q;
  queue_init(&q, SMALL_QUEUE_SIZE);
  // Generate test values and place the values in both the array and the queue.
  for (int i=0; i<SMALL_QUEUE_SIZE + FILLER; i++) {
    double value = (double)rand()/(double)RAND_MAX;
    queue_overwritePush(&q, value);
    testData[i] = value;
  }
  // Everything is initialized, compare the contents of the queue against the array.
  for (int i=0; i<SMALL_QUEUE_SIZE; i++) {
    double qValue = queue_readElementAt(&q, i);
    if (qValue != testData[i+FILLER]) {
      printf("testData[%d]:%lf != queue_readElementAt(&q, %d):%lf\n", i, testData[i+FILLER], i+FILLER, qValue);
      success = 0;
    }
  }
  if (!success) {
    printf("Test 1 failed. Array contents not equal to queue contents.\n");
  } else {
    printf("Test 1 passed. Array contents match queue contents.\n");
  }
  success = 1;  // Remain optimistic.
  // Test 2: test a chain of 5 queues against a single large queue that is the same size as the cumulative 5 queues.
  for (int i=0; i<SMALL_QUEUE_COUNT; i++)
    queue_init(&(smallQueue[i]), SMALL_QUEUE_SIZE);
  for (int i=0; i<SMALL_QUEUE_COUNT; i++) {
    for (int j=0; j<SMALL_QUEUE_SIZE; j++)
      queue_overwritePush(&(smallQueue[i]), 0.0);
  }
  queue_init(&largeQueue, SMALL_QUEUE_SIZE * SMALL_QUEUE_COUNT);
  for (int i=0; i<SMALL_QUEUE_SIZE*SMALL_QUEUE_COUNT; i++)
    queue_overwritePush(&largeQueue, 0.0);
  for (int i=0; i<TEST_ITERATION_COUNT; i++) {
    double newInput = (double)rand()/(double)RAND_MAX;
    popAndPushFromChainOfSmallQueues(newInput);
    queue_overwritePush(&largeQueue, newInput);
    if (!compareChainOfSmallQueuesWithLargeQueue(i)) {  // i is passed to print useful debugging messages.
      success = 0;
    }
  }

  if (success)
    printf("Test 2 passed. Small chain of queues behaves identical to single large queue.\n");
  else
    printf("Test 2 failed. The content of the chained small queues does not match the contents of the large queue.\n");
  return success;
}

