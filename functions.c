/*
 * functions.c
 *  This file holds the kernel functions. As it is part of the kernel, it can
 *  access kernel variables such as running process and mailbox lists.
 *
 *  Created on: Nov 4, 2019
 *      Author: Callum Cottrell
 */

#include "process.h"
#include <stdio.h>
#include "functions.h"
#include "cqueue.h"

#define TRIGGER_PENDSV 0x10000000
#define NVIC_INT_CTRL_R (*((volatile unsigned long *) 0xE000ED04))
#define TRUE 1
#define FALSE 0
extern struct pcb *running[6];
extern volatile int priorityLevel;
extern int registersSaved;
extern struct mailbox mboxList[100];
extern struct message *msgList;
extern queue *outQueue;

void removePCB();
void findNextProcess();
void processSwitch();
int deallocate();
struct message* allocate();

/* The end of a process. Frees all memory associated with the process*/
void k_terminate(){

    //unlink the PCB from the running queue
    removePCB();

    //Store the running PCB for freeing after next PCB found
    struct pcb *temp = running[priorityLevel];
    //Go to next - no way back to removed PCB from next
    if (running[priorityLevel] == running[priorityLevel]->next){
        //linked list is empty at this priority level.
        running[priorityLevel] = 0;
    }
    else
        running[priorityLevel] = running[priorityLevel]->next;

    // free stack from originally malloced base
    free((void *)temp->stackBase);

    // free the pointer
    free(temp);

    // find next process to run - if it priority level does not change,
    // next process begins naturally
    findNextProcess();

    // set the PSP to the new process' sp for returning from SVC
    set_PSP(running[priorityLevel]->SP);

}

/*Returns the PID of the currently running process*/
int k_getPID(){
    return running[priorityLevel]->PID;
}

//For handling prints to the screen
int k_print(struct CUPch *toPrint){
    char *string = (char*)toPrint;
    //Store the CUP in outQueue

    print(string);

    while (getSize(outQueue))
        transmitByte();

    printChar(toPrint->ch);
}

/*Changes the priority of the running process. Process can lower itself below the
 * current highest priority and as a result become
 * "blocked" (not truly blocked, still in waiting to run queue) */
int k_nice(unsigned int newPriority){

    //Cant be higher than 6 or less than 1. No point in continuing if same priority level
    if (newPriority == priorityLevel || newPriority < 1 || newPriority > 6){
        return -1;
    }

    //Store the current PSP
    running[priorityLevel]->SP = get_PSP();

    //Remove the running process from list of running processes
    removePCB();

    //Save the currently running process for after scheduling
    struct pcb *movingPCB = running[priorityLevel];

    //check if this was the last PCB in its p queue
    if (running[priorityLevel] == running[priorityLevel]->next)
        running[priorityLevel] = 0;
    else
        running[priorityLevel] = running[priorityLevel]->next;

    //Now the pcb can be added
    addPCB(movingPCB, newPriority);

    //Ensure that the current priority level is up-to-date
    findNextProcess();

    //Set the PSP to the new running process
    set_PSP(running[priorityLevel]->SP);

    return 1;
}

/* The kernel function that takes a mail box number and tries to bind it to the running process*/
int k_bind(unsigned int boxNumber){

    if (boxNumber > 100){
        return -1;
    }
    else if (!boxNumber){
       //find a free mailbox - not real time - will need fixing if i have time
        //Maybe a linked list of free mboxes...
        for (boxNumber = 1; boxNumber < 100; boxNumber++){
            //If there is no process using this mbox number
            if (!mboxList[boxNumber].process){
                mboxList[boxNumber].process = running[priorityLevel];
                return boxNumber;
            }
        }
    }
    // Accessing a specific mbox
    else {
        //access mboxList at boxNumber to see if its in use
        if (mboxList[boxNumber].process){
            //Fail - is in use
            return -1;
        }
        else {
            //Mbox is free. Store pointer to the running process in mbox
            mboxList[boxNumber].process = running[priorityLevel];
            return boxNumber;
        }

    }
    //Failed
    return -1;
}

int k_unbind(unsigned int boxNumber){

    //Can't unbind from number out of range
    if (boxNumber > 100 || !boxNumber){
        return -1;
    }
    // If the mbox list's process is the running process we can unbind
    else if (mboxList[boxNumber].process == running[priorityLevel]){
        //Free the messages that could have been stored
        while (mboxList[boxNumber].msg){
            deallocate(mboxList[boxNumber].msg);
            mboxList[boxNumber].msg = mboxList[boxNumber].msg->next;
        }
        mboxList[boxNumber].blocked = 0;
        mboxList[boxNumber].process = 0;
    }

}

/* Send a message to a the recvNum mailbox. */
int k_send(unsigned int recvNum, unsigned int srcNum, void *msg, unsigned int size){

    unsigned psize;

    //Make sure that the sender owns the mailbox and other mailbox exists
    if (running[priorityLevel] == mboxList[srcNum].process && mboxList[recvNum].process!=0){

       //If the process isnt blocked
       if (!mboxList[recvNum].process->blocked){

           //Allocate memory for the message coming in
           struct message *msgPtr = allocate();
           if (msgPtr){
               msgPtr->size = size;
               msgPtr->sender = srcNum;
               //msgPtr->data = (char *)malloc(size);

               // Copy the contents of the message into the newly allocated memory
               memcpy(msgPtr->data,msg,size);
               //Put the new message at the top of the list
               msgPtr->next = mboxList[recvNum].msg;
               mboxList[recvNum].msg = msgPtr;
           }
           //Failed due to lack of message space
           else return -1;

           }
       //The process is blocked and waiting for this message
        else {
            //determine what size of message to use - smaller is used
           if (size > mboxList[recvNum].msg->size){
               psize = mboxList[recvNum].msg->size;
           }
           else {
               psize = size;
           }
           //transfer the message to the mailbox
           memcpy(mboxList[recvNum].msg->msgLoc,msg,psize);
           *(mboxList[recvNum].msg->sender) = srcNum;

           //unblock the process that was trying to receive.
           addPCB(mboxList[recvNum].process, mboxList[recvNum].process->priority);
           //NEED to change this - addPCB was clobbering the RUNNING process.
           //running[priorityLevel] = running[priorityLevel]->next;
           mboxList[recvNum].process->regSaved = 0;
           mboxList[recvNum].process->blocked = 0;

           //Process has been added - need to determine the new priority level
           processSwitch(FALSE);
       }

    }
    //This mailbox does not belong to the running process!
    else {
        return -1;
    }

    return psize;
}

//Kernel receive function
int k_recv(unsigned int recvNum, unsigned int *src, void *msg, unsigned int size){

    unsigned int psize = -1;

    //Make sure that the calling process is the owner of the mailbox
    if (mboxList[recvNum].process == running[priorityLevel]){

        //There is a message waiting
        if (mboxList[recvNum].msg != 0){

            //Should I be able to receive from a specific process?
//            if (*src != 0){
//            struct message *top = mboxList[recvNum].msg;
//
//                while (mboxList[recvNum].msg){
//
//
//                }
//            }

            if (mboxList[recvNum].msg->size > size)
                psize = size;
            else
                psize = mboxList[recvNum].msg->size;

            //Copy the message stored in the mailbox
            memcpy(msg, mboxList[recvNum].msg->data, psize);

            //Save the sender
            *src = mboxList[recvNum].msg->sender;

            //"Free" the message
            deallocate(mboxList[recvNum].msg);

            //Can be null
            mboxList[recvNum].msg = mboxList[recvNum].msg->next;

        }
        // Nothing to receive. Block!
        else {
            //Allocate space for the anticipated message
            struct message *newMsg = allocate();
            newMsg->size = size;
            newMsg->next = 0;
           // newMsg->data = msg;
            //newMsg->data = malloc(size);
            newMsg->msgLoc = msg;
            newMsg->sender = src;

            mboxList[recvNum].msg = newMsg;
            running[priorityLevel]->blocked = 1;

            //Take the process out of the running queue
            removePCB();

            //Switch the PSP for popping new PC when returning from SVC
            processSwitch(TRUE);
       }
   }
    else {
        psize = -1;
    }

    return psize;
}

/* Saves the PSP of the running process, and changes the currently running process
To the next in the running queue, and sets the PSP to the SP of the new running process */
void nextProcess() {
    running[priorityLevel] -> SP = get_PSP();
    running[priorityLevel] = running[priorityLevel]->next;
    set_PSP(running[priorityLevel]->SP);
}

/*This unlinks the running PCB from the list of waiting to run processes
    When this function returns, running[pL] still points to the running process.
    Programmer must handle the process switching to the next. If the process was the only
    item in the list, this unlinking does nothing. The programmer must also handle that case*/
void removePCB (){
    running[priorityLevel]->prev->next = running[priorityLevel]->next;
    running[priorityLevel]->next->prev = running[priorityLevel]->prev;
}


// Add a PCB to the desired priority queue
void addPCB(struct pcb *new, int priority){

    //If the priority queue is empty
    if (!running[priority]){
        new -> next = new;
        new -> prev = new;
        running[priority]= new;
    }
    //Queue not empty, add in the new PCB
    else {
        new->next = running[priority]->next;
        new->next->prev = new;
        running[priority]->next = new;
        new->prev = running[priority];

    }
}

//Retrieve a message from the list of usable messages
struct message* allocate(){

    struct message *newMsg;
    if (!msgList){
        return 0;
    }
    newMsg = msgList;
    msgList = msgList->next;
    return newMsg;

}

//For giving a message back to the linked list of usable messages
int deallocate(struct message* oldMsg){
    oldMsg->next = msgList;
    msgList=oldMsg;
    return 1;
}

/* This function is called whenever there is a change to the priority queue of running
 * processes. The purpose of the function is to determine which queue to access when calling
 * running[priorityLevel] by finding the highest priorityLevel queue with a process within it
 */
void findNextProcess() {
    int i;
    for (i=0; i<6; i++){
        if (running[i])
            priorityLevel=i;
    }
}

/* This function allows for the switching of processes within the kernel. When
 * exiting the SVC call, the a new PSP will have been set, returning to
 * a different process than the one that called SVC originally.*/
void processSwitch(int pcbRemoved){

    //Store the current PSP
    running[priorityLevel]->SP = get_PSP();

    if (pcbRemoved){
    //check if this is the last PCB in its p queue
    if (running[priorityLevel] == running[priorityLevel]->next)
        running[priorityLevel] = 0;
    else
        running[priorityLevel] = running[priorityLevel]->next;
    }
    //Ensure that the current priority level is active
    findNextProcess();

    //Set the PSP to the new running process
    set_PSP(running[priorityLevel]->SP);

}

