/*
 * This file contains the processes that are registered to run in the OS.
 * It also contains helper functions for these processes.
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

//Prototypes
void assignR7(unsigned long);
int send(unsigned int dest, unsigned int src, void *msg, unsigned int size);
int recv(unsigned int dest, unsigned int *src, char *msg, unsigned int size);
int bind(unsigned int mboxNum);
int nice(unsigned int newPriority);
int printRequest(char *msg);
int printVT(int row, int col, char ch);
int inputRequest(char *input);
int pkCall(unsigned int code, void *arg);
int getPID();


//Process for taking input from the user, used for demonstrating
//mailbox functionality.
void IOprocess(){

   queue *commandQueue = malloc(sizeof(queue));
   initQueue(commandQueue);
   int UARTbox = 4;
   //Bind to mailbox 4
   bind(UARTbox);

   int any = 0;
   char data;
   char input[MAX_BUFFER_SIZE];
   char command[MAX_BUFFER_SIZE];
   int messageSize;
   char output[MAX_BUFFER_SIZE];
   int i;
   int quit = 0;
   printVT(30,0,'\0');
   printRequest("\r>");

   //While the user is still using the program
   while (!quit){

      //Request an update on user input
      messageSize = inputRequest(input);

      //recv(1, any, input, MAX_BUFFER_SIZE);

      //While there is user input data to use
      for (i = 0; i < messageSize; i++){

           //Remove the data from the input queue and determine how to handle it
           data = input[i];

           switch (data){
             //If the user hit the enter key
             case ENTER :{

                 //Form string for message
                 int j;
                 unsigned int size = getSize(commandQueue);

                 for (j = 0; j < size; j++)
                     command[j] = dequeue(commandQueue);

                 /*Terminate the command with a null character outside the
                 range of the original string*/
                 size++;
                 command[j] = '\0';

                 printRequest("\x1b[2K \r>");
                 send(6,UARTbox,(void*)command,size+1);
                 send(7,UARTbox,(void*)command,size+1);
                 //Wait until woken up by anybody
                 if (!strcmp("pause", command)){
                     printRequest(CLEAR_LINE);
                     printVT(20,2,'\0');
                     sprintf(output, "Now receiving on process # %d ", pkCall(GETID,0));
                     printRequest(output);
                     any = 0;
                     recv(UARTbox,&any,command,MAX_BUFFER_SIZE);
                 }

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

//Process that waits for process to output through IPC.
//Only purpose is to demonstrate ipc.
void outProcess(){
    //Bind to a mailbox
    int boxNum = 6;
    int otherBox = 7;
    int printColumn = 0;
    int printRow = 5;
    bind(boxNum);
    bind(otherBox);
    char msg[MAX_BUFFER_SIZE];
    char output[MAX_BUFFER_SIZE];
    int size;
    int sender = 1;
    printVT (4,0,'\0');
    sprintf(output, "PID:(%d) waiting for message from: %d", pkCall(GETID,0), sender);
    printRequest(output);
    while(1){
        //Restore cup
        printVT (30,2,'\0');
        //Wait for a message
        size = recv(boxNum,&sender,msg,80);
        //Check if it was a valid receive
        if (size > 0){
        printVT (printRow++,printColumn,'\0');
        if (printRow > 10)
            printRow = 5;
        //printRequest(CLEAR_LINE);
        sprintf(output, "msg from %d to %d:", sender, boxNum);
        printRequest(output);
        printRequest(msg);
        if (!strcmp(msg, "mbox 7") && boxNum == 6){
            unbind(boxNum);
            printColumn = 40;
            printRow = 5;
            //Now start receiving on the other box
            boxNum = otherBox;
        }
        else if (!strcmp(msg, "mbox 6") && boxNum == 7){
            boxNum = 6;
        }
        else if (!strcmp(msg, "nice lower")){
            nice(4);
        }
        }
        //if mbox cant be received from, do not keep checking
        else {
            break;
        }
    }

    sprintf(msg,"Terminating %d", pkCall(GETID,0));
    printVT(2,0,'\0');
    printRequest(msg);
    printVT(30,2,'\0');
}

//The process that always runs in the background
void idle() {
    int idleBox = 8;
    bind(idleBox);
    char idle = 'A';
    while (1){
        updateTime();
        /* Check for input data. If the user inputs any data, stop idling*/
        if (getSize(inQueue)){
            send(1,idleBox,&idle,1);
            //dequeue(inQueue);
        }
        //Every 1 second send a char
        if (time->tenths % 10  == 0){
        // Form a CUP to print in the top right
            printVT(0, 80, idle);
            printVT(30,2,'\0');
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


//Function that LR contains to for all processes
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
/* Send through IPC*/
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
    /* Since globals aren�t permitted, this must be done each call */
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

