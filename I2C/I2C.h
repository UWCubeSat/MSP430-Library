/*
 * I2C.h
 *
 *  Created on: Jan 16, 2020
 *      Author: Logan Ince
 */

#ifndef I2C_H_
#define I2C_H_

#include <msp430.h>
#include <stdint.h>
#include <bsp.c>

// Copy pasted from i2c.h
typedef enum _bus_instance_i2c {
    I2CBus1 = 1,
    I2CBus2 = 2,
} bus_instance_i2c;

typedef enum {
    i2cRes_noerror = 0,
    i2cRes_startTimeout = 1,
    i2cRes_stopTimeout = 2,
    i2cRes_nack = 3,
    i2cRes_transmitTimeout = 4
} i2c_result;

typedef struct _bus_registers_i2c {
    volatile uint16_t * UCBxCTLW0;
    volatile uint8_t * UCBxCTL1;
    volatile uint16_t * UCBxCTLW1;
    volatile uint16_t * UCBxBRW;
    volatile uint16_t * UCBxSTATW;
    volatile uint16_t * UCBxTBCNT;
    volatile uint16_t * UCBxRXBUF;
    volatile uint16_t * UCBxTXBUF;
    volatile uint16_t * UCBxI2CSA;
    volatile uint16_t * UCBxIE;
    volatile uint16_t * UCBxIFG;
    volatile uint16_t * UCBxIV;
} bus_registers_i2c;

// Copy pasted from i2c.h Low Level functions, need to find declaration of bus_instance_i2c
FILE_STATIC void inline i2cDisable(bus_instance_i2c bus)  { I2CREG(bus, UCBxCTLW0) |= UCSWRST; }
void inline i2cEnable(bus_instance_i2c bus)  { I2CREG(bus, UCBxCTL1) &= ~UCSWRST; }
FILE_STATIC void inline i2cMasterTransmitStart(bus_instance_i2c bus)  { I2CREG(bus, UCBxCTL1) |= UCTR | UCTXSTT; }
FILE_STATIC void inline i2cMasterTransmitStop(bus_instance_i2c bus) { I2CREG(bus, UCBxCTL1) |= UCTXSTP; }
FILE_STATIC void inline i2cMasterReceiveStart(bus_instance_i2c bus)  { I2CREG(bus, UCBxCTL1) &= ~UCTR;  I2CREG(bus, UCBxCTL1) |= UCTXSTT;  }
FILE_STATIC void inline i2cLoadTransmitBuffer(bus_instance_i2c bus, uint8_t input)  {  I2CREG(bus, UCBxTXBUF) = input; }
FILE_STATIC uint8_t inline i2cRetrieveReceiveBuffer(bus_instance_i2c bus)  {  return I2CREG(bus, UCBxRXBUF);  }
FILE_STATIC void inline i2cAutoStopSetTotalBytes(bus_instance_i2c bus, uint8_t count)  { I2CREG(bus, UCBxTBCNT) = count; }  // NOTE: must be called under reset!

// CONFIGM... configuration values are defined as MACROS
// Use these for values that will be needed in other initializers (best example:  buffer sizes)
// KEEP at 3. Only 3 built in pull-up resistors. without removing these 4+ instances will render voltage too low to measure
#define CONFIGM_i2c_maxperipheralinstances   3
#define CONFIGM_i2c_maxdevices               8

// Reads data
i2c_result i2cMasterRead(hDev device, uint8_t * buff, uint8_t szToRead);

// Writes data
i2c_result i2cMasterWrite(hDev device, uint8_t * buff, uint8_t szToWrite);

// Reads from Registers
i2c_result i2cMasterRegisterRead(hDev device, uint8_t registeraddr, uint8_t * buff, uint8_t szToRead);

// Reads and Writes data
i2c_result i2cMasterCombinedWriteRead(hDev device, uint8_t * wbuff, uint8_t szToWrite, uint8_t * rbuff, uint8_t szToRead);



// Reset all Previously Initialized I2C Buses.
void i2cReset();

// Initializes I2C
hDev i2cInit(bus_instance_i2c instance, uint8_t slaveaddr);


#endif /* I2C_H_ */
