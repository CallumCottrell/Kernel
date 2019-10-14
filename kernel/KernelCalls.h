#ifndef KERNELCALLS_H_
#define KERNELCALLS_H_

enum kernelcallcodes {STARTUP, GETID, NICE, TERMINATE};

//Kernel Call arguments.
struct kcallargs
{
unsigned int code; // Directs the kernel to the function to execute
unsigned int rtnvalue;
unsigned int arg1;
unsigned int arg2;
};

void SVCall(void);

#endif /*KERNELCALLS_H_*/
