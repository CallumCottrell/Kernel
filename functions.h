/*
 * Prototypes for the kernel functions.
 *
 *  Created on: Nov 4, 2019
 *      Author: Callum Cottrell
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

void k_terminate();

int k_getPID();

void k_printVT(void *toPrint);

void k_print(char *string);

void k_printChar(char byte);

int k_nice(unsigned int newPriority);

int k_unbind (unsigned int boxNum);

int k_bind(unsigned int boxNum);

int k_recv(unsigned int recvNum, unsigned int *src, void *msg, unsigned int size);

int k_send(unsigned int recvNum, unsigned int send, void *msg, unsigned int size);

void nextProcess();

void removePCB();

void nextProcess();

void addPCB();

#endif /* FUNCTIONS_H_ */
