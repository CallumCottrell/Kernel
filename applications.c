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


void UARTReceive(){
   int sender = 4;
   //Bind to mailbox 4
   bind(sender);

   char data;
   //A queue for storing specifically VT-100 commands
    queue VTQueueAddress;
    queue *VTQueue = &VTQueueAddress;
    initQueue(VTQueue);
   //The index of the buffer where the command started
   int vt100Command = 0;

   int senderID;
   void *msg;

   //recv(4, &senderID, msg, 10);

   print("\n\r>");
   //While the user is still using the program
   while (1){

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
                     send(5,sender,commandQueue->buffer,getSize(commandQueue));

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

void outProcess(){
    //Bind to a mailbox

    bind(6);
    void *msg;
    int size;

    while(1){
        size = recv(6,0,msg,80);
        pkCall(PRINT, msg);

    }

}
void goodbye(){

    //Bind to mailbox 5
    bind(5);

    int i =0;

    volatile int callum = 9;
    //char *hey = "hey";
    char msg[40] = "hey";

    unsigned int sender = 0;

    recv(5,&sender, msg, 40);

    if (sender == 4)
        nice(4);

}

//The process that always runs
void lowest() {

    //Allocate memory for the time struct and initalize variables
    Time timeAddress = {0,0,0,0};
    time = &timeAddress;

    //Allocate memory for the date struct and initialize variables
    Date dateAddress = {1,8,2019};
    date = &dateAddress;

    char idle = 'A';
    while (1){
        updateTime();
        if (time->seconds % 2 == 0){
        // Form a CUP to print in the top right
            printVT(3, 4, idle);
            idle++;
            if (idle == 'Z')
                idle = 'A';
        }
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
    uart_data . nul = '/0';
    uart_data . ch = ch;
    return pkCall(PRINT, &uart_data);
}

