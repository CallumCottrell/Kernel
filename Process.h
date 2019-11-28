/*
 * Process.h
 *
 *  Created on: Oct 13, 2019
 *      Author: callu
 */

#ifndef PROCESS_H_
#define PROCESS_H_

#define TRUE    1
#define FALSE   0
#define PRIVATE static

#define SVC()   __asm(" SVC #0")
#define disable()   __asm(" cpsid i")
#define enable()    __asm(" cpsie i")

#define MSP_RETURN 0xFFFFFFF9    //LR value: exception return using MSP as SP
#define PSP_RETURN 0xFFFFFFFD    //LR value: exception return using PSP as SP
#define ESC 27
void set_LR(volatile unsigned long);
unsigned long get_PSP();
void set_PSP(volatile unsigned long);
unsigned long get_MSP(void);
void set_MSP(volatile unsigned long);
unsigned long get_SP();

void volatile save_registers();
void volatile restore_registers();

#define CLEAR_SCREEN    "\x1b[2J"
#define CURSOR_SAVE     "\x1b""7"
#define CURSOR_HOME     "\x1b[H"
#define CLEAR_LINE      "\x1b[2K"
#define HOME_COLOURS    "\x1b[0;30;47m"
#define CURSOR_MIDDLE   "\x1b[20C"
#define TERM_COLOURS    "\x1b[0;0;0m"
#define CURSOR_RESTORE  "\x1b""8"

#define CURSOR_LEFT     "\x1b[D"
#define CURSOR_RIGHT    "\x1b[C"
#define CURSOR_UP       "\x1b[A"
#define CURSOR_DOWN     "\x1b[B"

#define STACKSIZE   1024

/* Cortex default stack frame */
struct stack_frame
{
/* Registers saved by s/w (explicit) */
/* There is no actual need to reserve space for R4-R11, other than
 * for initialization purposes.  Note that r0 is the h/w top-of-stack.
 */
unsigned long r4;
unsigned long r5;
unsigned long r6;
unsigned long r7;
unsigned long r8;
unsigned long r9;
unsigned long r10;
unsigned long r11;
/* Stacked by hardware (implicit)*/
unsigned long r0;
unsigned long r1;
unsigned long r2;
unsigned long r3;
unsigned long r12;
unsigned long lr;
unsigned long pc;
unsigned long psr;
};

/* Process control block */

struct pcb
{
/* Stack pointer - r13 (PSP) */
unsigned int SP;
unsigned int PID;
unsigned int stackBase;
unsigned int priority;
int regSaved;
int blocked;
/* Links to adjacent PCBs */
struct pcb *next;
struct pcb *prev;
};

// Interprocess Communication
struct message{
    unsigned int *sender;
    void *msgLoc;
    char data[80];
    int size;
    struct message *next;
};

struct mailbox {
    struct pcb *process;
    struct message *msg;
    char busy;
    int blocked;
};

struct CUPch
{
char esc;
char sqrbrkt;
char line[2];   /* 01 through 24 */
char semicolon;
char col[2];    /* 01 through 80 */
char cmdchar;
char nul;
char ch;
};

#endif /*PROCESS_H_*/
