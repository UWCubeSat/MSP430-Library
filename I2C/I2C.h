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

//
hDev i2cInit(bus_instance_i2c instance, uint8_t slaveaddr);


#endif /* I2C_H_ */
