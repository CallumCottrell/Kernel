/*
 *  The header file for cqueue.c. This contains the function prototypes for use by
 *  other files in the program as well as the definition of the queueStruct.
 *  Created on: Sep 20, 2019
 *      Author: Callum Cottrell
 */

#ifndef CQUEUE_H_
#define CQUEUE_H_

#define MAX_BUFFER_SIZE 80

/* A queue with a write and read pointer than can store a maximum
 * of 80 characters. */
typedef struct queueStruct {
    char buffer[MAX_BUFFER_SIZE]; //how do i store each character?
    unsigned int write; // Position of the writing index
    unsigned int read; // Position of the reading index

} queue;

extern void enqueue(queue *q, char newChar);

extern char dequeue(queue *q);

extern void backspace(queue *q);

extern void initQueue(queue *q);

extern void clearQueue(queue *q);

extern unsigned int getSize(queue *q);

#endif /* CQUEUE_H_ */
