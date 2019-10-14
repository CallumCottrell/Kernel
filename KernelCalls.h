#ifndef KERNELCALLS_H_
#define KERNELCALLS_H_

enum kernelcallcodes {GETID, NICE, TERMINATE};

struct kcallargs
{
unsigned int code;
unsigned int rtnvalue;
unsigned int arg1;
unsigned int arg2;
};

#endif /*KERNELCALLS_H_*/
