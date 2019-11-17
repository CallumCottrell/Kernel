/*
 * applications.c
 *
 *  Created on: Nov 3, 2019
 *      Author: callum cottrell B00712510
 */

#include <stdio.h>
#include "process.h"
#include "KernelCalls.h"
#include <stdlib.h>
#include "UART.h"
#include "SysTick.h"
#include "applications.h"
#include "cqueue.h"
#include "time.h"
#include "UART.h"

// Global variables
extern Date *date;
extern Time *time;
extern queue *inQueue;
extern queue *commandQueue;
extern queue *outQueue;


extern int alarmPtr;
extern int alarmTenthSeconds;

void assignR7(unsigned long);
volatile int helloValue = 0;

int send(unsigned int dest, unsigned int src, void *msg, unsigned int size);
int recv(unsigned int dest, unsigned int *src, void *msg, unsigned int size);
int bind(unsigned int mboxNum);

int pkCall(unsigned int code, void *arg);


void UARTReceive(){

   //Bind to mailbox 4
   bind(4);

   // Queue for holding the chars received over uart
   queue uartInBufferAddress;
   inQueue = &uartInBufferAddress;
   initQueue(inQueue);

   // A queue that stores the command entered by the user
   queue commandQueueAddress;
   commandQueue = &commandQueueAddress;
   initQueue(commandQueue);

   //A queue for storing specifically VT-100 commands
   queue VTQueueAddress;
   queue *VTQueue = &VTQueueAddress;
   initQueue(VTQueue);

   //If this queue has data stored to it, it will be transmitted
   queue outQueueBufferAddress;
   outQueue = &outQueueBufferAddress;
   initQueue(outQueue);

   //Allocate memory for the time struct and initalize variables
   Time timeAddress = {0,0,0,0};
   time = &timeAddress;

   //Allocate memory for the date struct and initialize variables
   Date dateAddress = {1,8,2019};
   date = &dateAddress;

   char data;

   //The index of the buffer where the command started
   int vt100Command = 0;

   int senderID;
   void *msg;

   //recv(4, &senderID, msg, 10);

   print("\n\r>");
   //While the user is still using the program
   while (1){

       //Block this process and wait for UART info
       //recv(4, &senderID, msg, 80);
       //If the input queue was updated form the string
       //If the timer is set to go off, alarmPtr will be set
       //if the output queue is full output a character
       while (!getSize(inQueue) && !alarmPtr && !getSize(outQueue));

//       if (alarmPtr){
//           printAlarm();
//           alarmPtr = 0;
//       }

       if (getSize(outQueue)){
           transmitByte();
       }

       else if (getSize(inQueue)) {

           //Remove the data from the input queue and determine how to handle it
           data = dequeue(inQueue);

           switch (data){
             //If the user hit the enter key
             case ENTER :
                 //if the user is entering a VT100 command, run the command
                 if (vt100Command){
                     enqueue(VTQueue, NUL);
                     //Send the command
                     print(VTQueue->buffer);
                     //Done executing command
                     vt100Command = FALSE;

                     clearQueue(VTQueue);

                 }
                 //else, the user entered time, date or alarm.
                 else
                     //processCommand();
                     recv(4,&senderID,commandQueue->buffer,getSize(commandQueue));
                 break;
            // Hitting the ESC key triggers a VT100 command
             case ESCAPE :
                 // Variable to give command to putty when enter key hit
                 vt100Command = TRUE;
                 //oldIndex = inQueue->write - 1;
                 enqueue(VTQueue, data);
                 break;

             //If the user hits backspace, remove entry from the command queue
             case BACKSPACE :
                 if (!vt100Command){
                     backspace(commandQueue);
                     printChar(data);
                 }
                 else
                     backspace(VTQueue);
                 break;

             //Regular character entry is default. determine where to enqueue
             default :
                 if (!vt100Command){
                     //Form the string for parsing
                     enqueue(commandQueue, data);
                     //Store the data in the output queue
                     enqueue(outQueue, data);
                 }
                 else {
                     //Do not output the data, store the VT-100 command
                     enqueue(VTQueue, data);
                 }
           }
       }

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
    //Bind to mailbox 5
    bind(5);

    int i =0;
    volatile int callum = 9;
    char *hey = "hey";
    void *msg = (void*)hey;
    int sender;
    send(4,5, msg, 80);
//    for (i=0;i<5;i++)
//        helloValue++;

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
