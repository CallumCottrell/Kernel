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

int send(unsigned int dest, unsigned int src, void *msg, unsigned int size);
int recv(unsigned int dest, unsigned int *src, void *msg, unsigned int size);
int bind(unsigned int mboxNum);

int pkCall(unsigned int code, void *arg);


void hello(){
   helloValue = 5;

//   for (i=0;i<5;i++)
//       helloValue++;
   //Bind to mailbox 4
   bind(4);
   int senderID;
   void *msg;

   recv(4, &senderID, msg, 10);


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
//    for (i=0;i<5;i++)
//        helloValue++;
    while(1){
        helloValue--;
    }
  //  SVC();
}

//The process that always runs
void lowest() {
int i;
    for (i=0;i<1000000;i++){
        helloValue++;
    }

}

//Function that LR points to for all processes
void terminate(){
    struct kCallArgs kArgs;
    kArgs.code = TERMINATE;
    assignR7((volatile unsigned long) &kArgs);
    SVC();
}

void assignR7(volatile unsigned long pointer){
    __asm(" mov r7, r0");
}

int pkCall(unsigned int code, void *arg){

    struct kCallArgs kArgs;
    kArgs.code = code;
    kArgs.arg1 = (unsigned int)arg;

    assignR7((volatile unsigned long) &kArgs);
    SVC();
    return kArgs.rtnvalue;

}

int send(unsigned int dest, unsigned int src, void *msg, unsigned int size){
    struct messageStruct pmsg;
    pmsg.destMb = dest;
    pmsg.srcMb = src;
    pmsg.msg = msg;
    pmsg.size = size;
    return pkCall(SEND, &pmsg);


}

//Try to receive from the mailbox at index destination, from the source mailbox (will be returned), the message stored in msg, and size
int recv(unsigned int dest, unsigned int *src, void *msg, unsigned int size){
    struct messageStruct pmsg;
    pmsg.destMb = dest;
    pmsg.srcMb = src;
    pmsg.msg = msg;
    pmsg.size = size;
    return pkCall(RECV, &pmsg);

}

int bind(unsigned int mboxNum){
    return pkCall(BIND, mboxNum);
}
