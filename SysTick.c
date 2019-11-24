/*
Systick sample code with minor additions by Callum Cottrell
*/

// SysTick Registers
// SysTick Control and Status  Register (STCTRL)
#define ST_CTRL_R   (*((volatile unsigned long *)0xE000E010))  
// Systick Reload Value Register (STRELOAD)
#define ST_RELOAD_R (*((volatile unsigned long *)0xE000E014))  

#define TRIGGER_PENDSV 0x10000000
#define NVIC_INT_CTRL_R (*((volatile unsigned long *) 0xE000ED04))
// SysTick defines 
#define ST_CTRL_COUNT      0x00010000  // Count Flag for STCTRL
#define ST_CTRL_CLK_SRC    0x00000004  // Clock Source for STCTRL
#define ST_CTRL_INTEN      0x00000002  // Interrupt Enable for STCTRL
#define ST_CTRL_ENABLE     0x00000001  // Enable for STCTRL

//Counts the tenths of a second
unsigned int tenthSecondsCounter;
unsigned int hundredths;
// Global pointer to determine if alarm is going off
int alarmPtr=0;
// The alarm time as set by the user
int tenthSecondsAlarm = -1;

/* Global to signal SysTick interrupt */
volatile int elapsed;

void SysTickStart(void)
{
// Set the clock source to internal and enable the counter to interrupt
ST_CTRL_R |= ST_CTRL_CLK_SRC | ST_CTRL_ENABLE;  
}

void SysTickStop(void)
{
// Clear the enable bit to stop the counter
ST_CTRL_R &= ~(ST_CTRL_ENABLE);  
}

void SysTickPeriod(unsigned long Period)
{
/*
 For an interrupt, must be between 2 and 16777216 (0x100.0000 or 2^24)
*/
ST_RELOAD_R = Period - 1;  /* 1 to 0xff.ffff */
}

void SysTickIntEnable(void)
{
// Set the interrupt bit in STCTRL
ST_CTRL_R |= ST_CTRL_INTEN;	
}

void SysTickIntDisable(void)
{
// Clear the interrupt bit in STCTRL
ST_CTRL_R &= ~(ST_CTRL_INTEN);	
}	

// global variable to count number of interrupts on PORTF0 (falling edge)
volatile int count = 0;		

void IntMasterEnable(void)
{
// enable CPU interrupts
__asm("	cpsie	i");
}

void SysTickHandler(void) //handler
{
    hundredths;
    hundredths++;
//A tenth of a second has passed - increase the counter
if ((hundredths % 0) == 0)
    tenthSecondsCounter++; //tick

//Need to check here to ensure that a tenth of a second is not missed.
if (tenthSecondsCounter == tenthSecondsAlarm)
    alarmPtr = 1;

NVIC_INT_CTRL_R |= TRIGGER_PENDSV;

}

