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


extern int alarmPtr;
extern int alarmTenthSeconds;
// Global variables
extern Date *date;
extern Time *time;
extern queue *inQueue;
extern queue *commandQueue;
extern queue *outQueue;

void assignR7(unsigned long);
volatile int helloValue = 0;

int send(unsigned int dest, unsigned int src, void *msg, unsigned int size);
int recv(unsigned int dest, unsigned int *src, char *msg, unsigned int size);
int bind(unsigned int mboxNum);
int nice(unsigned int newPriority);
int printRequest(char *msg);
int printVT(int row, int col, char ch);

int pkCall(unsigned int code, void *arg);


void IOprocess(){
   int UARTbox = 4;
   //Bind to mailbox 4
   bind(UARTbox);

   char data;
   char *string;
   int messageSize;
   int senderID;
   void *msg;
   int i;

   printVT(12,0,'\0');
   printRequest("\r>");
   //While the user is still using the program
   while (1){

       //If the input queue was updated form the string
       //If the timer is set to go off, alarmPtr will be set
       //if the output queue is full output a character
       //while (!getSize(inQueue) && !getSize(outQueue));

       messageSize = recv(UARTbox,0, msg, 80);

       string = (char*)msg;

      for (i = 0; i < messageSize; i++){

           //Remove the data from the input queue and determine how to handle it
           data = string[i];

           switch (data){
             //If the user hit the enter key
             case ENTER :

                 send(6,UARTbox,commandQueue->buffer,getSize(commandQueue));
                 //Wait until woken up by anybody
                 recv(4,0, msg, 80);
                 break;

             //If the user hits backspace, remove entry from the command queue
             case BACKSPACE :

                 backspace(commandQueue);
                 printRequest(data);

                 break;

             //Regular character entry is default. determine where to enqueue
             default :

                 //Form the string for parsing
                 enqueue(commandQueue, data);
                 //Store the data in the output queue
                // enqueue(outQueue, data);

                 printRequest(data);


           }
       }

   }


}


void outProcess(){
    //Bind to a mailbox

    bind(6);
    void *msg;
    int size;

    while(1){
        printVT (5,0,'\0');
        printRequest("outProcess waiting...");
        size = recv(6,0,msg,80);
        printVT (5,0,'\0');
        printRequest(msg);
    }

}
void inProcess(){

    //Bind to mailbox 5
    bind(5);
    const char *commandString = "block all";

    int i =0;

    volatile int callum = 9;
    //char *hey = "hey";
    char msg[80];

    unsigned int sender = 0;
    while(1){

    recv(5,&sender, msg, 80);

    }

}

//The process that always runs
void lowest() {
    bind(8);
    char idle = 'A';
    while (1){
        updateTime();
        /* Check for input data. If the user inputs any data, stop idling*/
        if (getSize(inQueue))
            send(6,8,inQueue->buffer,getSize(inQueue));

        //Every 2 seconds, send idle
        if (time->tenths % 10  == 0){
        // Form a CUP to print in the top right
            printVT(0, 80, idle);
            printVT(12,0,'\0');
            idle++;
            //Once the alphabet is completed
            if (idle == '[')
                idle = 'A';
        }
        //Ensure only one sent per second
        while (time->tenths % 10 == 0)
            updateTime();

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
/* */
int send(unsigned int dest, unsigned int src, void *msg, unsigned int size){
    struct messageStruct pmsg;
    pmsg.destMb = dest;
    pmsg.srcMb = src;
    pmsg.msg = msg;
    pmsg.size = size;
    return pkCall(SEND, &pmsg);

}

//Try to receive from the mailbox at index destination, from the source mailbox (will be returned), the message stored in msg, and size
int recv(unsigned int dest, unsigned int *src, char *msg, unsigned int size){
    struct messageStruct pmsg;
    pmsg.destMb = dest;
    pmsg.srcMb = src;
    pmsg.msg =(void*) msg;
    pmsg.size = size;
    return pkCall(RECV, &pmsg);

}

int bind(unsigned int mboxNum){
    return pkCall(BIND, mboxNum);
}

int unbind(unsigned int mboxNum){
    return pkCall(UNBIND, mboxNum);
}

int nice(unsigned int newPriority){
    return pkCall(NICE, newPriority);
}

int printRequest(char *msg){
    return pkCall(PRINT, msg);
}

int printVT(int row, int col, char ch)
{
/* Output a single character to specified screen position */
/* CUP (Cursor position) command plus character to display */
/* Assumes row and col are valid (1 through 24 and 1 through 80, respectively) */
struct CUPch uart_data;
    /* Since globals aren’t permitted, this must be done each call */
    uart_data . esc = ESC;
    uart_data . sqrbrkt = '[';
    uart_data . line[0] = '0' + row / 10;
    uart_data . line[1] = '0' + row % 10;
    uart_data . semicolon = ';';
    uart_data . col[0] = '0' + col / 10;
    uart_data . col[1] = '0' + col % 10;
    uart_data . cmdchar = 'H';
    uart_data . nul = '\0';
    uart_data . ch = ch;
    return pkCall(PRINTVT, &uart_data);
}

