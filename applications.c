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

void assignR7(unsigned long);

volatile int helloValue = 0;

void hello(){
   helloValue = 5;
   int i = 0;
//   for (i=0;i<5;i++)
//       helloValue++;

   while (1){
       helloValue++;
   }
}

void check(){

    volatile int wow;
    volatile int wow2;

}

void check2(){

    volatile int wow;
    volatile int wow2;

}
void goodbye(){
    helloValue = 20000;
    int i =0;
    volatile int callum = 9;
    for (i=0;i<5;i++)
        helloValue++;
//    while(1){
//        helloValue--;
//    }
  //  SVC();
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
    assignR7((volatile unsigned long) &args);
    SVC();

    //Call SVC
    //Change the LR to the PC of the next process??? will it do this automatically?
}

void assignR7(volatile unsigned long pointer){
    __asm(" mov r7, r0");
}
