/*
 * This file is the data structure that is integral to the receiving and transmitting of
 * data from the user through UART. It is a circular queue that has a maximum size of 80 bytes.
 *
 *  Created on: Sep 20, 2019
 *      Author: Callum Cottrell
 */


#include "cqueue.h"

//Store a char to the queue and increase the write buffer.
void enqueue(queue *q, char newChar){

    if (q->write - q->read == MAX_BUFFER_SIZE ){ // Full?
        return;
    }

    q->buffer[q->write++] = newChar; // Store the character in the buffer
    q->write = q->write % MAX_BUFFER_SIZE;// reset to 0 when the max is hit

}

//Return a character from the queue and increase the read index
char dequeue(queue *q){

    volatile char character;

    //Return the character at read
    character = q->buffer[q->read++];

    //Delete the character that was just returned
    q->buffer[q->read - 1] = 0;

    //Wrap around if read surpasses the maximum
    q->read = q->read % MAX_BUFFER_SIZE;
    return character;
}

//Remove the most recent post
void backspace(queue *q){
    // Write incremented after erroneous letter - has to go back 2 indices to get to the letter
    q->write--;
    if (q->write < 0){
        q->write = 0;
    }
    q->buffer[q->write] = '\0';
    //How do you handle going backwards to 128
  //  q->write = q->write % MAX;

}

//Initialize the queue.
void initQueue(queue *q){

    q->write = 0;
    q->read = 0;

}

void clearQueue(queue *q){

    while (q->read != q->write) {
        q->buffer[q->read++] = '\0';
        q->read = q->read % MAX_BUFFER_SIZE;
    }
    //If the queue is cleared, might as well restart read and write...
    q->read = 0;
    q->write = 0;


}
//Return the difference in index pointers to find the size.
unsigned int getSize(queue *q){
    return (q->write - q->read) % MAX_BUFFER_SIZE;
}







