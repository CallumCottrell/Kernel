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

void assignR7(unsigned long);
volatile int helloValue = 0;

int send(unsigned int dest, unsigned int src, void *msg, unsigned int size);
int recv(unsigned int dest, unsigned int *src, char *msg, unsigned int size);
int bind(unsigned int mboxNum);
int nice(unsigned int newPriority);
int printRequest(char *msg);
int printVT(int row, int col, char ch);
int inputRequest(char *input);
int pkCall(unsigned int code, void *arg);


void IOprocess(){

   queue *commandQueue = malloc(sizeof(queue));
   initQueue(commandQueue);
   int UARTbox = 4;

   //Bind to mailbox 4
   bind(UARTbox);

   char data;
   char input[MAX_BUFFER_SIZE];
   char command[MAX_BUFFER_SIZE];
   int messageSize;
   int i;
   int quit = 0;
   printVT(12,0,'\0');
   printRequest("\r>");

   //While the user is still using the program
   while (!quit){

      //Request an update on user input
      messageSize = inputRequest(input);

      //While there is user input data to use
      for (i = 0; i < messageSize; i++){

           //Remove the data from the input queue and determine how to handle it
           data = input[i];

           switch (data){
             //If the user hit the enter key
             case ENTER :{

                 //Form string for message
                 int j;
                 int size = getSize(commandQueue);

                 for (j = 0; j < size; j++)
                     command[j] = dequeue(commandQueue);
                 command[j] = 0;
                 send(6,UARTbox,(void*)command,size);
                 //Wait until woken up by anybody
                 break;
             }
             //If the user hits backspace, remove entry from the command queue
             case BACKSPACE :

                 backspace(commandQueue);
                 pkCall(PRINTCHAR, data);

                 break;

             //Regular character entry is default. determine where to enqueue
             default :
                 //Form the string for parsing
                 enqueue(commandQueue, data);
                 //Store the data in the output queue
                // enqueue(outQueue, data);
                 pkCall(PRINTCHAR, data);
           }
       }

   }


}

//Expects a message of size equal to the queue size
int inputRequest(char *message){
    return pkCall(GETUART, message);
}

void outProcess(){
    //Bind to a mailbox

    bind(6);
    char msg[80];
    int size;
    int sender;
    while(1){
        printVT (5,0,'\0');
        printRequest("outProcess waiting...");
        printVT (12,1,'\0');
        size = recv(6,&sender,msg,80);
        printVT (6,0,'\0');
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

