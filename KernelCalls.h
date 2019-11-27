#ifndef KERNELCALLS_H_
#define KERNELCALLS_H_

enum kernelcallcodes {STARTUP, GETID, NICE, TERMINATE, BIND,
    UNBIND, SEND, RECV, PRINT, PRINTVT, GETUART, PRINTCHAR};

//Kernel Call arguments.
struct kCallArgs
{
unsigned int code; // Directs the kernel to the function to execute
unsigned int rtnvalue;
unsigned int arg1;
};

struct messageStruct {
    //Destination mailbox index
    unsigned int destMb;
    //Source mailbox index
    unsigned int *srcMb;
    //The message
    void *msg;

    unsigned int size;
};

void SVCall(void);
extern void pendSVHandler();

#endif /*KERNELCALLS_H_*/
