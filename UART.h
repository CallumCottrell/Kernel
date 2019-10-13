/*
 * UART.h
 * A header file for UART.c
 *  Created on: Sep 20, 2019
 *      Author: Callum Cottrell
 */

#ifndef UART_H_
#define UART_H_

extern void UART0_Init();           // Initialize UART0
extern void InterruptEnable(unsigned long InterruptIndex);
extern void UART0_IntEnable(unsigned long flags);
extern void UART0_IntHandler(void);
extern void InterruptMasterEnable(void);
extern void print(char *string);
extern void printChar(char byte);
extern void transmitByte();

#define INT_VEC_UART0           5           // UART0 Rx and Tx interrupt index (decimal)
#define UART_INT_TX             0x020       // Transmit Interrupt Mask
#define UART_INT_RX             0x010       // Receive Interrupt Mask
#define UART_INT_RT             0x040       // Receive Timeout Interrupt Mask

#define TRUE 1
#define FALSE 0

//The maximum size of the output queue
#define MAX_OUTPUT_LENGTH 25

//ASCII Constants
#define BACKSPACE 127
#define ENTER 13
#define ESCAPE 0x1b
#define NUL '\0'

#endif /* UART_H_ */
