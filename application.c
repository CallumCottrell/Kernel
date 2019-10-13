/*
 * This file contains the main of the entire program. The purpose of the program is to
 * keep track of time for the user. The user can specify the time, date, or an alarm. The
 * user can also request the time and date to be displayed. The main first initializes
 * its data structures and interrupts. It then polls its input, output, and alarm timer
 * for updates. If a queue has a size greater than 0, the appropriate action is taken place.
 * Once the user hits the ENTER key, the input is processed by a separate file (time.c).
 *
 *
 *  Created on: Sep 20, 2019
 *      Author: Callum Cottrell
 */


#include <stdio.h>
#include <string.h>
#include "time.h"
#include "UART.h"
#include "cqueue.h"
#include "SysTick.h"

//Function prototype for processing commands after enter key
void processCommand();

// Global variables
extern Date *date;
extern Time *time;
extern queue *inQueue;
extern queue *commandQueue;
extern queue *outQueue;

extern int alarmPtr;
extern int alarmTenthSeconds;

void main(void){

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

    /* Initialize UART */
    UART0_Init();           // Initialize UART0
    InterruptEnable(INT_VEC_UART0);       // Enable UART0 interrupts
    UART0_IntEnable(UART_INT_RX | UART_INT_TX); // Enable Receive and Transmit interrupts
    InterruptMasterEnable();      // Enable Master (CPU) Interrupts
    /* Initialize Systick */
    SysTickPeriod(MAX_WAIT);
    SysTickIntEnable();
    SysTickStart();

    char data;

    //The index of the buffer where the command started
    int vt100Command = 0;

    print("\n\r>");
    //While the user is still using the program
    while (1){

        //If the input queue was updated form the string
        //If the timer is set to go off, alarmPtr will be set
        //if the output queue is full output a character
        while (!getSize(inQueue) && !alarmPtr && !getSize(outQueue));

        if (alarmPtr){
            printAlarm();
            alarmPtr = 0;
        }

        else if (getSize(outQueue)){
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
                      processCommand();
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


void processCommand(){
        int fail = TRUE;
        //Determine how to analyze the command based on first letter.
        switch (commandQueue->buffer[commandQueue->read]){

        //Time commands
        case 't' :
        case 'T' :
            fail = readTime();
            break;

        //Alarm commands
        case 'a':
        case 'A':
            fail = readAlarm();
            break;
        //Date commands
        case 'd':
        case 'D':
            fail = readDate();
            break;

        //Command failed
        default:

        }

        //If input failed give error message
        if (fail)
            print("\n\r?");

        print("\n\r>");
        clearQueue(commandQueue);

    }

