/**

 * Author: Emad Khan, ECED4402 TA
 * Summer 2017
 *
 * Minor additions by Callum Cottrell
 * Sept 22 2019
 */
#include <stdio.h>
#include "cqueue.h"
#include <stdlib.h>
#include "UART.h"
// UART0 & PORTA Registers
#define GPIO_PORTA_AFSEL_R  (*((volatile unsigned long *)0x40058420))   // GPIOA Alternate Function Select Register
#define GPIO_PORTA_DEN_R    (*((volatile unsigned long *)0x4005851C))   // GPIOA Digital Enable Register
#define GPIO_PORTA_PCTL_R   (*((volatile unsigned long *)0x4005852C))   // GPIOA Port Control Register
#define UART0_DR_R          (*((volatile unsigned long *)0x4000C000))   // UART0 Data Register
#define UART0_FR_R          (*((volatile unsigned long *)0x4000C018))   // UART0 Flag Register
#define UART0_IBRD_R        (*((volatile unsigned long *)0x4000C024))   // UART0 Integer Baud-Rate Divisor Register
#define UART0_FBRD_R        (*((volatile unsigned long *)0x4000C028))   // UART0 Fractional Baud-Rate Divisor Register
#define UART0_LCRH_R        (*((volatile unsigned long *)0x4000C02C))   // UART0 Line Control Register
#define UART0_CTL_R         (*((volatile unsigned long *)0x4000C030))   // UART0 Control Register
#define UART0_IFLS_R        (*((volatile unsigned long *)0x4000C034))   // UART0 Interrupt FIFO Level Select Register
#define UART0_IM_R          (*((volatile unsigned long *)0x4000C038))   // UART0 Interrupt Mask Register
#define UART0_MIS_R         (*((volatile unsigned long *)0x4000C040))   // UART0 Masked Interrupt Status Register
#define UART0_ICR_R         (*((volatile unsigned long *)0x4000C044))   // UART0 Interrupt Clear Register
#define UART0_CC_R          (*((volatile unsigned long *)0x4000CFC8))   // UART0 Clock Control Register

#define UART_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART_RX_FIFO_ONE_EIGHT  0x00000038  // UART Receive FIFO Interrupt Level at >= 1/8
#define UART_TX_FIFO_SVN_EIGHT  0x00000007  // UART Transmit FIFO Interrupt Level at <= 7/8
#define UART_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART_CTL_UARTEN         0x00000301  // UART RX/TX Enable
#define UART_CTL_EOT            0x00000010  // UART End of Transmission Enable
#define EN_RX_PA0               0x00000001  // Enable Receive Function on PA0
#define EN_TX_PA1               0x00000002  // Enable Transmit Function on PA1
#define EN_DIG_PA0              0x00000001  // Enable Digital I/O on PA0
#define EN_DIG_PA1              0x00000002  // Enable Digital I/O on PA1

// Clock Gating Registers
#define SYSCTL_RCGCGPIO_R      (*((volatile unsigned long *)0x400FE608))
#define SYSCTL_RCGCUART_R      (*((volatile unsigned long *)0x400FE618))

#define SYSCTL_RCGCGPIO_UART0      0x00000001  // UART0 Clock Gating Control
#define SYSCTL_RCGCUART_GPIOA      0x00000001  // Port A Clock Gating Control

// Clock Configuration Register
#define SYSCTRL_RCC_R           (*((volatile unsigned long *)0x400FE0B0))

#define CLEAR_USRSYSDIV     0xF83FFFFF  // Clear USRSYSDIV Bits
#define SET_BYPASS      0x00000800  // Set BYPASS Bit

#define NVIC_EN0_R      (*((volatile unsigned long *)0xE000E100))   // Interrupt 0-31 Set Enable Register
#define NVIC_EN1_R      (*((volatile unsigned long *)0xE000E104))   // Interrupt 32-54 Set Enable Register

#define disable() __asm(" cpsid i")
#define enable() __asm(" cpsie i")

/* Globals */
volatile char Data;     /* Input data from UART receive */
queue *inQueue; //Data REC
queue *outQueue; //Data to XMIT

//Initialize UART and pass the alarm flag for polling in talk
void UART0_Init()
{
    volatile int wait;
    /* Initialize UART0 */
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCUART_GPIOA;   // Enable Clock Gating for UART0
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCGPIO_UART0;   // Enable Clock Gating for PORTA
    wait = 0; // give time for the clocks to activate

    UART0_CTL_R &= ~UART_CTL_UARTEN;        // Disable the UART
    wait = 0;   // wait required before accessing the UART config regs

    // Setup the BAUD rate
    UART0_IBRD_R = 8;   // IBRD = int(16,000,000 / (16 * 115,200)) = 8.680555555555556
    UART0_FBRD_R = 44;  // FBRD = int(.680555555555556 * 64 + 0.5) = 44.05555555555556

    UART0_LCRH_R = (UART_LCRH_WLEN_8);  // WLEN: 8, no parity, one stop bit, without FIFOs)

    GPIO_PORTA_AFSEL_R = 0x3;        // Enable Receive and Transmit on PA1-0
    GPIO_PORTA_PCTL_R = (0x01) | ((0x01) << 4);         // Enable UART RX/TX pins on PA1-0
    GPIO_PORTA_DEN_R = EN_DIG_PA0 | EN_DIG_PA1;        // Enable Digital I/O on PA1-0

    UART0_CTL_R = UART_CTL_UARTEN;        // Enable the UART
    wait = 0; // wait; give UART time to enable itself.

}

void InterruptEnable(unsigned long InterruptIndex)
{
/* Indicate to CPU which device is to interrupt */
if(InterruptIndex < 32)
    NVIC_EN0_R = 1 << InterruptIndex;       // Enable the interrupt in the EN0 Register
else
    NVIC_EN1_R = 1 << (InterruptIndex - 32);    // Enable the interrupt in the EN1 Register
}

void UART0_IntEnable(unsigned long flags)
{
    /* Set specified bits for interrupt */
    UART0_IM_R |= flags;
}

void UART0_IntHandler(void)
{
/*
 * Simplified UART ISR - handles receive and xmit interrupts
 * Application signaled when data received
 */
    if (UART0_MIS_R & UART_INT_RX)
    {
        /* RECV done - clear interrupt and make char available to application */
        UART0_ICR_R |= UART_INT_RX;
        enqueue(inQueue, UART0_DR_R);
    }

    if (UART0_MIS_R & UART_INT_TX)
    {
        /* XMIT done - clear interrupt */
        UART0_ICR_R |= UART_INT_TX;
    }
}

void InterruptMasterEnable(void)
{
    /* enable CPU interrupts */
    enable();
}

//Enqueue the contents of a string for output
void print(char *string){
    int i = 0;
    while (string[i] != '\0')
        enqueue(outQueue, string[i++]);
}

//Prints one byte to the screen (useful for backspace)
void printChar(char byte){
    disable();
    if(!(UART0_FR_R & UART_FR_TXFF))
    UART0_DR_R = byte;

    enable();
}

//Dequeue
void transmitByte(void) {

    disable();
    if(!(UART0_FR_R & UART_FR_TXFF))
    UART0_DR_R = dequeue(outQueue);

    enable();
}

