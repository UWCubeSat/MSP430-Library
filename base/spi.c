#include <stdio.h>
#include <msp430.h>
#include <stdint.h>
#include "spi.h"

void spiInit(uint8_t csPins)
{
   /* 1. Set UCSWRST. */
	UCA2CTLW0 = UCSWRST;
	UCA2CTLW0 |= UCSSEL_2;
	UCA2CTLW0 |= UCMODE_THREE_WIRE_SPI | UCMSB | UCSYNC | UCMST | UCCKPH;
	//UCCKPH shifts bits to rising edge of clock.
   
   /* 2. Initialize all eUSCI registers with UCSWRST = 1 (including UCxCTL1). */

	/* 3. Configure ports. 
	P5SEL0.0 = 1: UCB1SIMO
	P5SEL0.1 = 1: UCB1SOMI
	P5SEL0.2 = 1: UCB1CLK */
	P5SEL0 |= BIT4 | BIT5 | BIT6;
		
	/* Dividing SMCLK by SPI_CLOCK_DIV. */
	/* SMCLK is originally running at 8 MHz. */
	UCA2BRW = SPI_CLOCK_DIV;

	/* 4. Ensure that any input signals into the SPI module such as UCxSOMI (in master mode) 
	or UCxSIMO and UCxCLK (in slave mode) have settled to their final voltage levels 
	before clearing UCSWRST and avoid any unwanted transitions during operation. */
	uint16_t i;
	for (i = 0; i < WAIT_TIME; i++)
	{
	}
	
	/* 5. Clear UCSWRST to start device. */
	UCA2CTLW0 &= ~UCSWRST;

	/* 6. Enable interrupts with UCRXIE or UCTXIE. */
}
}