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

extern struct pcb *running[5];
extern volatile int priorityLevel;
extern int registersSaved;
extern struct mailbox mboxList[100];

void k_terminate(){

    //unlink the PCB from the running queue
    removePCB();

    //Is running bound to any mailboxes? free mailboxe


    //Store the running PCB for freeing after next PCB found
    struct pcb *temp = running[priorityLevel];
    //Go to next - no way back to removed PCB from next
    running[priorityLevel] = running[priorityLevel]->next;

    // free stack from originally malloced base
    free((void *)running[priorityLevel]->stackBase);

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
            //fail
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

    //Make sure that the sender owns the mailbox
    if (running[priorityLevel] == mboxList[srcNum].process){

       //If the process isnt blocked
       if (!mboxList[recvNum].process->blocked){
           //Allocate memory for the message coming in
           msgPtr = malloc(size);
           // Copy the contents of the message into the newly allocated memory
           memcpy(msgPtr,msg,size);
           //Make a new message struct for the linked list
           struct message *newMsg = malloc(sizeof(struct message));
           //Store the message
           newMsg->data = (char *)msgPtr;
           newMsg->sender = srcNum;

           //Append that message to linked list of messages at the receiving mailbox
           addMsg(newMsg,recvNum);

           }
       //The process is blocked and waiting for this message
        else {
           if (size < mboxList[recvNum].msg->)
            //Give the message directly to the receiver
           memcpy(mboxList[recvNum].msg,msg,size);

       }

    }
    //This mailbox does not belong to the running process!
    else {

    }

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
            memcpy(msg, mboxList[recvNum].msg, psize);
            // unlink mboxLise[recvNum].msg
            // free the message
            return psize;
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
    new->next = running[priority]->next;
    running[priority]->next = new;
    new->prev = running[priority];
    running[priority] = new;
    }
}

void addMsg(struct message *newMsg, unsigned int recvNum){

    //If the mailbox message is empty
    if (!mboxList[recvNum].msg){
        newMsg -> next = newMsg;
        newMsg -> prev = newMsg;
        mboxList[recvNum].msg= newMsg;
    }
    //Queue not empty, add in the new PCB
    else {
    newMsg->next = mboxList[recvNum].msg->next;
    mboxList[recvNum].msg->next = newMsg;
    newMsg->prev = mboxList[recvNum].msg;
    mboxList[recvNum].msg = newMsg;
    }
}



void findNextProcess() {
    int i;
     for ( i=0; i< 5; i++){
         if (running[i])
             priorityLevel=i;
     }
}
