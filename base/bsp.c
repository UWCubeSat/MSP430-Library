#include "bsp.h"

void bspI2CInit() {
   WDTCTL |= WDTPW | WDTHOLD;       // stop watchdog timer
   
   UCB0CTL1 |= UCSWRST;             // ensure eUSCI-B0 is reset before configuration
   UCB0CTLW0 |= UCMODE_3 + UCMST;   // I2C master mode
   UCB0BRW = BAUDRATE;                 // baud rate = SMCLK / 8
   UCB0CTLW0 = UCASTP_2;            // automatic STOP assertion
   UCB0TBCNT = TX;                // TX 7 bytes of data
   UCB0I2CSA = SLVADD;              // address slave is 12hex
   P2SEL |= I2CPN;                   // configure I2C pins
   UCB0CTL &= ^UCSWRST;             // puts eUSCI-B0 in operational state
   UCB0IE |= UCTXIE;                // enable TX-interrupt
   GIE;                             // enables general interrupt
   
   UCB0TXBUF = TXBFFR;                // fill TX buffer
   
   // Disable the GPIO power-on default high-impedance mode to activate previously
   // configured port settings
   PM5CTL0 &= ~LOCKLPM5;
}