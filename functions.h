/*
 * functions.h
 *
 *  Created on: Nov 4, 2019
 *      Author: Callum Cottrell
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

void k_terminate();

int k_getPID();

void k_nice();

int k_bind(unsigned int boxNum);

int k_recv(unsigned int recvNum, void *msg, unsigned int size);

int k_send();

void nextProcess();

void removePCB();

void nextProcess();

void addPCB();

#endif /* FUNCTIONS_H_ */
