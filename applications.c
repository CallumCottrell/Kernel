/*
 * applications.c
 *
 *  Created on: Nov 3, 2019
 *      Author: callu
 */

#include <stdio.h>
#include "process.h"
#include "KernelCalls.h"
#include <stdlib.h>
#include "UART.h"
#include "SysTick.h"
#include "applications.h"

void assignR7(struct kCallArgs *k);

volatile int helloValue = 0;

void hello(){
   helloValue = 5;
   int i = 0;
   for (i=0;i<5;i++)
       helloValue++;
}

void goodbye(){
    helloValue = 20000;
    int i =0;
    for (i=0;i<5;i++)
        helloValue++;
}

void lowest() {
int i;
    for (i=0;i<10000000;i++){
        helloValue++;
    }

}

void terminate(){
    struct kCallArgs args;
    args.code = TERMINATE;
    assignR7((unsigned long) &args);
    SVC();

    //Call SVC
    //Change the LR to the PC of the next process??? will it do this automatically?
}

void assignR7(unsigned long pointer){
    __asm(" mov r7, r0");
}
