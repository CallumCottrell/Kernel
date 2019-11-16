/*
 * Kernel code: Supervisor call (SVC) handler example
 * ECED 4402
 *
 * 04 Oct 2017 - Comments in SVCHandler()'s "second call"
 * 11 Mar 2013 - Handle passing argument to kernel by register
 * 05 Mar 2013 - Fix first SVC call to access PSP correctly
 *             - Updated comments
 * 20 Feb 2012 - First version
 */
#include <stdio.h>
#include "process.h"
#include "KernelCalls.h"
#include <stdlib.h>
#include "UART.h"
#include "SysTick.h"
#include "applications.h"
#include "functions.h"

#define TRIGGER_PENDSV 0x10000000
#define NVIC_INT_CTRL_R (*((volatile unsigned long *)0xE000ED04))
#define NVIC_SYS_PRI3_R (*((volatile unsigned long*)0xE000ED20))
#define PENDSV_LOWEST_PRIORITY 0X00E00000
#define THUMB_MODE 0x01000000

//A pcb that is running for each priority level
struct pcb *running[5]={0,0,0,0,0};
volatile int priorityLevel;
volatile int registersSaved;
struct mailbox mboxList[100];
struct message *msgList;
struct pcb *blocked;

int regProcess();
void initKernel();

void main (void) {

   regProcess(hello, 1000, 4);
   regProcess(goodbye, 1001, 4);
   regProcess(lowest, 10, 0);
   initKernel();
   SVC();

}

//Set the priority level to lowest by default and pcb pointers to null.
void initKernel(){
    //Find the process to run first
    findNextProcess();

    registersSaved = 0;
    /* Initialize UART */
    UART0_Init();           // Initialize UART0
    InterruptEnable(INT_VEC_UART0);       // Enable UART0 interrupts
    UART0_IntEnable(UART_INT_RX | UART_INT_TX); // Enable Receive and Transmit interrupts
    InterruptMasterEnable();      // Enable Master (CPU) Interrupts
    /* Initialize Systick */
    SysTickPeriod(MAX_WAIT);
    SysTickIntEnable();
    SysTickStart();

    //Set pendsv to lowest priority
    NVIC_SYS_PRI3_R |= PENDSV_LOWEST_PRIORITY;
    int i;
    msgList = malloc(sizeof(struct message));
    struct message *top = msgList;
    //create the messages
    for (i=0;i<64;i++){
        msgList->next = malloc(sizeof(struct message));
        msgList = msgList->next;
    }
    //terminate with a null
    msgList->next = 0;
    //point to the top of the message list
    msgList = top;
}


void pendSVHandler(){

    if (!registersSaved){
     save_registers();
     registersSaved = 0;
     }
     nextProcess();
     restore_registers();

}

//Trying to pass readTime() to this
//Register a process that may be used. Only called at the initialization stage, for each process.
int regProcess(void (*func_name)(), unsigned int pid, unsigned int priority) {

    //Make a new PCB for this process to have its place in the queues
    struct pcb *newPCB = malloc(sizeof(struct pcb));
    newPCB->PID = pid;
    newPCB->priority = priority;
    //Add the PCB to its priority queue
    addPCB(newPCB, priority);

    //Allocate 512 bytes for the process' stack
    unsigned int stackPointer = malloc(1024*sizeof(unsigned char));
    newPCB->stackBase = stackPointer;
    stackPointer = stackPointer + (1024 * sizeof(unsigned char));
    stackPointer = stackPointer - (16 * sizeof(unsigned int));

    //Stack pointer currently points to the base of the stack.
    struct stack_frame *pStack = (struct stack_frame *)stackPointer;

    //Set the stack pointer .  V important !
    newPCB->SP = (unsigned int) pStack;

    pStack->psr = THUMB_MODE; // Thumb mode
    pStack->pc = (unsigned int)func_name; // Begin executing the function with PC = start of func
    pStack->lr = (unsigned int)&terminate;
    return 1;
}



void SVCall(void)
{
/* Supervisor call (trap) entry point
 * Using MSP - trapping process either MSP or PSP (specified in LR)
 * Source is specified in LR: F9 (MSP) or FD (PSP)
 * Save r4-r11 on trapping process stack (MSP or PSP)
 * Restore r4-r11 from trapping process stack to CPU
 * SVCHandler is called with r0 equal to MSP or PSP to access any arguments
 */

/* Save LR for return via MSP or PSP */
__asm("     PUSH    {LR}");

/* Trapping source: MSP or PSP? */
__asm("     TST     LR,#4");    /* Bit #4 indicates MSP (0) or PSP (1) */
__asm("     BNE     RtnViaPSP");

/* Trapping source is MSP - save r4-r11 on stack (default, so just push) */
__asm("     PUSH    {r4-r11}");
__asm("     MRS r0,msp");
__asm("     BL  SVCHandler");   /* r0 is MSP */
__asm("     POP {r4-r11}");
__asm("     POP     {PC}");

/* Trapping source is PSP - save r4-r11 on psp stack (MSP is active stack) */
__asm("RtnViaPSP:");
__asm("     mrs     r0,psp");
__asm("     stmdb   r0!,{r4-r11}"); /* Store multiple, decrement before */
__asm("     msr psp,r0");
__asm("     BL  SVCHandler");   /* r0 Is PSP */

/* Restore r4..r11 from trapping process stack  */
__asm("     mrs     r0,psp");
__asm("     ldmia   r0!,{r4-r11}"); /* Load multiple, increment after */
__asm("     msr psp,r0");
__asm("     POP     {PC}");

}

void SVCHandler(struct stack_frame *argptr)
{
/*
 * Supervisor call handler
 * Handle startup of initial process
 * Handle all other SVCs such as getid, terminate, etc.
 * Assumes first call is from startup code
 * Argptr points to (i.e., has the value of) either:
   - the top of the MSP stack (startup initial process)
   - the top of the PSP stack (all subsequent calls)
 * Argptr points to the full stack consisting of both hardware and software
   register pushes (i.e., R0..xPSR and R4..R10); this is defined in type
   stack_frame
 * Argptr is actually R0 -- setup in SVCall(), above.
 * Since this has been called as a trap (Cortex exception), the code is in
   Handler mode and uses the MSP
 */
static int firstSVCcall = TRUE;
struct kCallArgs *kcaptr;

if (firstSVCcall)
{
/*
 * Force a return using PSP
 * This will be the first process to run, so the eight "soft pulled" registers
   (R4..R11) must be ignored otherwise PSP will be pointing to the wrong
   location; the PSP should be pointing to the registers R0..xPSR, which will
   be "hard pulled"by the BX LR instruction.
 * To do this, it is necessary to ensure that the PSP points to (i.e., has) the
   address of R0; at this moment, it points to R4.
 * Since there are eight registers (R4..R11) to skip, the value of the sp
   should be increased by 8 * sizeof(unsigned int).
 * sp is increased because the stack runs from low to high memory.
*/
    set_PSP(running[priorityLevel] -> SP + 8 * sizeof(unsigned int));

    firstSVCcall = FALSE;
    /* Start SysTick */
    //systick_init();

    /*
     - Change the current LR to indicate return to Thread mode using the PSP
     - Assembler required to change LR to FFFF.FFFD (Thread/PSP)
     - BX LR loads PC from PSP stack (also, R0 through xPSR) - "hard pull"
    */
    __asm(" movw    LR,#0xFFFD");  /* Lower 16 [and clear top 16] */
    __asm(" movt    LR,#0xFFFF");  /* Upper 16 only */
    __asm(" bx  LR");          /* Force return to PSP */
}
else /* Subsequent SVCs */
{
/*
 * kcaptr points to the arguments associated with this kernel call
 * argptr is the value of the PSP (passed in R0 and pointing to the TOS)
 * the TOS is the complete stack_frame (R4-R10, R0-xPSR)
 * in this example, R7 contains the address of the structure supplied by
    the process - the structure is assumed to hold the arguments to the
    kernel function.
 * to get the address and store it in kcaptr, it is simply a matter of
   assigning the value of R7 (arptr -> r7) to kcaptr
 */


    kcaptr = (struct kCallArgs *) argptr -> r7;
    switch(kcaptr -> code){

    case TERMINATE:
            k_terminate();
            break;

    case GETID:
           kcaptr->rtnvalue = k_getPID();
           break;
    case BIND:
           kcaptr->rtnvalue = k_bind(kcaptr->arg1);
           break;

    case SEND:
    {
           struct messageStruct *m;
           m = (struct messageStruct*)kcaptr->arg1;
           kcaptr->rtnvalue = k_send(m->destMb, m->srcMb, m->msg, m->size);
           break;
    }
    default:
        kcaptr -> rtnvalue = -1;
    }

}

}
