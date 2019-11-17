/*
 * functions.c
 *  This file holds the kernel functions
 *
 *  Created on: Nov 4, 2019
 *      Author: Callum Cottrell
 */

#include "process.h"
#include <stdio.h>
#include "functions.h"

#define TRIGGER_PENDSV 0x10000000
#define NVIC_INT_CTRL_R (*((volatile unsigned long *) 0xE000ED04))

extern struct pcb *running[6];
extern volatile int priorityLevel;
extern int registersSaved;
extern struct mailbox mboxList[100];
extern struct message *msgList;

void removePCB();

void k_terminate(){

    //unlink the PCB from the running queue
    removePCB();

    //Is running bound to any mailboxes? free mailboxes


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

    // find next process to run - if it priority level does not change, next process begins naturally
    findNextProcess();

    // set the PSP to the new process' sp
    set_PSP(running[priorityLevel]->SP);

}



int k_getPID(){
    return running[priorityLevel]->PID;
}

void k_nice(){

}

int k_bind(unsigned int boxNumber){

    if (boxNumber > 100){
        return -3;
    }
    else if (!boxNumber){
       //find a free mailbox
    }
    else {
        //access mboxList at boxNumber to see if its in use
        if (mboxList[boxNumber].process){
            //fail - is in use
            return -1;
        }
        else {
            mboxList[boxNumber].process = running[priorityLevel];
            return boxNumber;
        }

    }
}

int k_send(unsigned int recvNum,unsigned int srcNum, void *msg, unsigned int size){

    unsigned psize;
    void *msgPtr;

    //Make sure that the sender owns the mailbox and other mailbox exists
    if (running[priorityLevel] == mboxList[srcNum].process && mboxList[recvNum].process!=0){

       //If the process isnt blocked
       if (!mboxList[recvNum].process->blocked){

           //Allocate memory for the message coming in
           struct message *msgPtr = allocate();
           msgPtr->size = size;
           msgPtr->sender = srcNum;
           msgPtr->data = malloc(size);
           // Copy the contents of the message into the newly allocated memory
           memcpy(msgPtr->data,msg,size);
           //Put the new message at the top of the list
           msgPtr->next = mboxList[recvNum].msg;
           mboxList[recvNum].msg = msgPtr;


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
           memcpy(mboxList[recvNum].msg->data,msg,psize);
           mboxList[recvNum].msg->sender = srcNum;

           //unblock the process that was trying to receive.
           addPCB(mboxList[recvNum].process, mboxList[recvNum].process->priority);
           //NEED to change this - addPCB was clobbering the RUNNING process.
           //running[priorityLevel] = running[priorityLevel]->next;
           mboxList[recvNum].process->regSaved = 1;
           mboxList[recvNum].process->blocked = 0;

          // findNextProcess();

           NVIC_INT_CTRL_R |= TRIGGER_PENDSV;
       }

    }
    //This mailbox does not belong to the running process!
    else {
        return -1;
    }

    return psize;
}

//Kernel receive function
int k_recv(unsigned int recvNum, void *msg, unsigned int size){

    unsigned int psize;

    //Make sure that the calling process is the owner of the mailbox
    if (mboxList[recvNum].process == running[priorityLevel]){

        //Is there anything to receive?
        if (mboxList[recvNum].msg != 0){
            if (mboxList[recvNum].msg->size > size)
                psize = size;
            else
                psize = mboxList[recvNum].msg->size;

            //Copy the message stored in the mailbox
            memcpy(msg, mboxList[recvNum].msg->data, psize);
            // unlink mboxLise[recvNum].msg
            // free the message
            return psize;
        }
        // Nothing to receive. Block!
        else {
            //Allocate space for the anticipated message
            struct message *newMsg = allocate();
            newMsg->size = size;
            newMsg->next = 0;
            newMsg->data = malloc(size);
            mboxList[recvNum].msg = newMsg;
            running[priorityLevel]->blocked = 1;

            //Take the process out of the running queue
            removePCB();

            running[priorityLevel]->SP = get_PSP();
            //check if this is the last item in the linked list
            if (running[priorityLevel] == running[priorityLevel]->next){
                running[priorityLevel] = 0;
                //Running from a new priority queue now
                findNextProcess();
            }
            else
            running[priorityLevel] = running[priorityLevel]->next;

            //The registers are saved from the SVC call in recv
            set_PSP(running[priorityLevel]->SP);
            registersSaved = 1;


        }
        }


}

void nextProcess() {
    running[priorityLevel] -> SP = get_PSP();
    running[priorityLevel] = running[priorityLevel]->next;
    set_PSP(running[priorityLevel]->SP);
}


void removePCB (){
    //unlink from list
    running[priorityLevel]->prev->next = running[priorityLevel]->next;
    running[priorityLevel]->next->prev = running[priorityLevel]->prev;

}


// Add a pcb to its priority queue
void addPCB(struct pcb *new, int priority){

    //If the priority queue is empty
    if (!running[priority]){
        new -> next = new;
        new -> prev = new;
        running[priority]= new;
    }
    //Queue not empty, add in the new PCB
    else {
    struct pcb *last = running[priority]->prev;

    new->next = running[priority]->next;
    new->next->prev = new;
    running[priority]->next = new;
    new->prev = running[priority];
    //running[priority] = new;
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

void findNextProcess() {
    int i;
     for ( i=0; i<=5; i++){
         if (running[i])
             priorityLevel=i;
     }
}
