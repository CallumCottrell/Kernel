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
    //remove PCB
    removePCB();

    struct pcb *temp = running[priorityLevel];
    running[priorityLevel] = running[priorityLevel]->next;

    // free memory
    free((void *)running[priorityLevel]->stackBase);
    free(temp);

    // find next process to run
    findNextProcess();
    registersSaved = 1;
    set_PSP(running[priorityLevel]->SP);

}



int k_getPID(){
    return running[priorityLevel]->PID;
}

void k_nice(){

}

int k_bind(unsigned int boxNumber){
    static struct mboxStatic = mboxList[0];
    static int freebox;
    if (boxNumber > 100){
        return -3;
    }
    else if (!boxNumber){
        mboxStatic.
    }
    else {
        //access mboxList at boxNumber to see if its in use
        if (mboxList[boxNumber]->process){
            //fail
            return -1;
        }
        else {
            mboxList[boxNumber]->process = running[priorityLevel];

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

void findNextProcess() {
    int i;
     for ( i=0; i< 5; i++){
         if (running[i])
             priorityLevel=i;
     }
}
