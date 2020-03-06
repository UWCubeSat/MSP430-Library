#include <stdio.h>
#include <msp430.h>
#include <stdint.h>
#include "spi.h"

void spiInit(uint8_t csPins)
{
   WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
   
   /* 1. Set UCSWRST. */
	UCA2CTLW0 = UCSWRST;
	UCA2CTLW0 |= UCSSEL_2;
	UCA2CTLW0 |= UCMODE_THREE_WIRE_SPI | UCMSB | UCSYNC | UCMST | UCCKPH;
	//UCCKPH shifts bits to rising edge of clock.
   
   /* 2. Initialize all eUSCI registers with UCSWRST = 1 (including UCxCTL1). */
   

	/* 3. Configure GPIO ports. 
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
   __interrupt void USCI_B1_ISR(void)
   {
      switch(__even_in_range(UCB1IV, USCI_SPI_UCTXIFG))
      {
         case USCI_NONE: break;
         case USCI_SPI_UCRXIFG:
            RXData = UCB1RXBUF;
            UCB1IFG &= ~UCRXIFG;
            __bic_SR_register_on_exit(LPM0_bits); // Wake up to setup next TX
            break;
         case USCI_SPI_UCTXIFG:
            UCB1TXBUF = TXData;                   // Transmit characters
            UCB1IE &= ~UCTXIE;
            break;
         default: break;
      }
   }
   
   void spiTransceive(uint8_t *pTxBuf, uint8_t *pRxBuf, size_t num, uint8_t csPin){
	   // Clear the MSP430's rxBuffer of any junk data left over from previous transactions.
	   //*pRxBuf = UCB1RXBUF;

	   uint8_t dummydata = UCB1RXBUF;

	   // TX all data from the provided register over the SPI bus by adding it to the txbuffer one byte at a time.
	   // Store all data received from the slave in pRxBuf.
	   uint8_t i;
	   for(i = 0; i < num; i++) {

         // Wait for any previous tx to finish.
	      // The timeout is used just in case an interrupt put us
	      // in an illegal state. Shouldn't happen, as we've been mitigating
	      // it, but we've kept it here is a last resort.
      
         uint16_t timeout = UINT16_MAX;
		   while (!(UCA2IFG & UCTXIFG) && timeout){
		   	timeout--;
		   }

		   // Drop CS Pin if it hasn't been dropped yet.
		   if (i == 0){
            if(csPin & CS_1){
               P2OUT &= ~BIT4;
            }
            else if(csPin & CS_2){
               P4OUT &= ~BIT4;
            }
            else if(csPin & CS_3){
               P4OUT &= ~BIT5;
            }
		   }
		   // Write to tx buffer.
		   UCA2TXBUF = *pTxBuf;
   
		   // Bring CS High again.
		   if (i == num - 1){
            if(csPin & CS_1){
               P2OUT |= BIT4;
            }
            else if(csPin & CS_2){
               P4OUT |= BIT4;
            } 
            else if(csPin & CS_3){
               P4OUT |= BIT5;
            }
		   }

		   // Wait for any previous rx to finish rx-ing.
         // The timeout is used just in case an interrupt put us
         // in an illegal state. Shouldn't happen, as we've been mitigating
         // it, but we've kept it here is a last resort.
         timeout = UINT16_MAX;
		   while (!(UCA2IFG & UCRXIFG) && timeout){
		   	timeout--;
		   }
		   // Store data transmitted from the slave.
		   *pRxBuf = UCA2RXBUF;

		   // Increment the pointer to send and store the next byte.
		   pTxBuf++;
		   pRxBuf++;
	   }
   }
}