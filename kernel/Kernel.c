/*
 * Kernel.c
 *
 *  Created on: Oct 13, 2019
 *      Author: callum
 */
#include "cqueue.h"
queue *priority1;

void initPCBs() {
    priority1 = malloc(sizeof(struct queueStruct));
}


int getPID();

void nice();
