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
extern void systick_init();

//A pcb that is running for each priority level
struct pcb *running[5];
int priorityLevel;

//Function prototypes
void hello();
int regProcess();
void initKernel();
void addPCB(struct pcb *new, int priority);
int helloValue = 0;

void main (void) {

   regProcess(hello, 1000, 4);
   initKernel();
   SVC();

}


//Trying to pass readTime() to this
//Register a process that may be used. Only called at the initialization stage, for each process.
int regProcess(void (*func_name)(), unsigned int pid, unsigned int priority) {

    //Allocate 512 bytes for the process' stack
    unsigned int *stackPointer = (unsigned int *)malloc(512*sizeof(unsigned int));
    //Stack pointer currently points to the base of the stack.
    struct stack_frame *pStack = (struct stack_frame *)stackPointer;
    //Make a new PCB for this process to have its place in the queues
    struct pcb *newPCB = malloc(sizeof(struct pcb));
    //Add the PCB to its priority queue
    addPCB(newPCB, priority);
    //Set the stack pointer
    newPCB->SP = (unsigned int) pStack;

    pStack->psr = 0x01000000; // Thumb mode
    pStack->pc = (unsigned int)func_name; // Begin executing the function with PC = start of func

    return 1;
}


void hello(){
   helloValue = 5;
}

void terminate(){

}


//Set the priority level to lowest by default and pcb pointers to null.
void initKernel(){
    int i;
    for ( i=0; i< 5; i++){
        if (running[i] !=0)
            priorityLevel=i;
    }
    volatile int check;
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

void schedule() {

}

void addProcess(int pid, int priority){

}

int getPID();

void nice();


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
struct kcallargs *kcaptr;

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


    kcaptr = (struct kcallargs *) argptr -> r7;
    switch(kcaptr -> code)
    {
//    case KERNEL_FUNCTION_XXX:
//        kcaptr -> rtnvalue = do_function_xxx();
//    break;
    default:
        kcaptr -> rtnvalue = -1;
    }

}

}
