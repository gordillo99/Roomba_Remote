#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "os.h"

volatile int isFull(volatile int *QCount);
volatile int isEmpty(volatile int *QCount);
void enqueue(volatile PD **p, volatile PD **Queue, volatile int *QCount);
volatile PD *dequeue(volatile PD **Queue, volatile int *QCount);

#endif /* _QUEUE_H_ */
