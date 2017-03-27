#include "queue.h"

/*
 *  Checks if queue is full
 */
volatile int isFull(volatile int *QCount) {
    return *QCount == MAXTHREAD - 1;
}

/*
 *  Checks if queue is empty
 */
volatile int isEmpty(volatile int *QCount) {
    return *QCount == 0;
}

/*
 *  Insert into the queue sorted by priority
 */
void enqueue(volatile PD **p, volatile PD **Queue, volatile int *QCount) {
    if(isFull(QCount)) {
        return NULL;
    }

    int i = (*QCount) - 1; // start from highest index 
    volatile PD *new = *p;
    volatile PD *temp = Queue[i];

    while(i >= 0) { // shift elements
        Queue[i+1] = Queue[i];
        i--;
        temp = Queue[i];
    }

	Queue[0] = *p; // put process at lowest index (end of queue)
    (*QCount)++; //increase count
}



/*
 *  Return the first element of the Queue
 */
volatile PD *dequeue(volatile PD **Queue, volatile int *QCount) {
   if(isEmpty(QCount)) {
	   return NULL;
   }

   volatile PD *result = (Queue[(*QCount)-1]); // grab process at highest index (start of queue)
   Queue[(*QCount)-1] = 0; // this line was added to remove elements from the queue
   (*QCount)--;

   return result;
}

/*
 * enqueueing function specialized for periodic processes, 
 *	it compares time to execute (countdown) before placing on queue
 */
void enqueuePeriodic(volatile PD **p, volatile PD **Queue, volatile int *QCount) {
	if(isFull(QCount)) {
		return;
	}

	int i = (*QCount) - 1; // start at highest index

	volatile PD *new = *p;

	volatile PD *temp = Queue[i];

	while(i >= 0 && (new->countdown >= temp->countdown)) { // iterate until finding right spot 
		Queue[i+1] = Queue[i];
		i--;
		temp = Queue[i];
	}

	Queue[i+1] = *p; // set process at right spot
	(*QCount)++; // increase counter
}

/*
 * returns value of first element in queue
 */
volatile PD *peek(volatile PD **Queue, volatile int *QCount) {
	if(isEmpty(QCount)) {
		return NULL;
	}
	volatile PD *result = (Queue[(*QCount)-1]); // first element in queue (highest index)
	return result;
}