#ifndef KERNELCALLS_H_
#define KERNELCALLS_H_

enum kernelcallcodes {STARTUP, GETID, NICE, TERMINATE};

//Kernel Call arguments.
struct kCallArgs
{
unsigned int code; // Directs the kernel to the function to execute
unsigned int rtnvalue;
unsigned int arg1;
unsigned int arg2;
};

void SVCall(void);
extern void pendSVHandler();
#endif /*KERNELCALLS_H_*/
