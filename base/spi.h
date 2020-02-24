//Use eUSCI_A for SPI and eUSCI_B for I2C
#ifndef SPI_H
#define SPI_H

#include <msp430.h>

// Configuration macros for UCB0CTLW0.
#define UCMODE_THREE_WIRE_SPI 				 0x0000
#define UCMODE_FOUR_WIRE_SPI_STE_ACTIVE_HIGH 0x0200
#define UCMODE_FOUR_WIRE_SPI_STE_ACTIVE_LOW  0x0400
#define UCMODE_I2C			  				 0x0600

// Clock Divider/other timing macros
#define SPI_CLOCK_DIV 						 0x0000
#define WAIT_TIME 			  				 0xFFFF


// These pins correspond to the chip select pins on our MSP CAN Block.
#define CS_1                                  0x01
#define CS_2                                  0x02
#define CS_3                                  0x04

uint8_t spiWriteInProgress;

/**
 * @brief Initialize SPI registers required to start transceiving.
 * @details Sets up the control block to reset the device.
 * 			Configure ports necessary for pins.
 * 			Sets up prescaler for clock, which is fed in at 8 MHz.
 * 			Wait for signals to settle.
 * 			Start device.
 * 			
 */
void spiInit(uint8_t csPins);

#endif	/* SPI_H */