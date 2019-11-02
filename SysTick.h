/*
 * SysTick.h
 *
 *  Created on: Sep 19, 2019
 *      Author: Callum Cottrell
 */

#ifndef SYSTICK_H_
#define SYSTICK_H_

extern void SysTickHandler(void) ;//handler
extern void IntMasterEnable(void);
extern void SysTickIntDisable(void);
extern void SysTickIntEnable(void);
extern void SysTickPeriod(unsigned long Period);
extern void SysTickStop(void);
extern void SysTickStart(void);
extern void clock(void);

// For an interrupt every tenth of a second
#define MAX_WAIT           0x27100

#endif /* SYSTICK_H_ */
