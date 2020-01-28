/*
 * I2C.c
 *
 *  Created on: Jan 16, 2020
 *      Author: Logan Ince
 */

#include "I2C.h";

// Initializes I2C
// UCB0CTL (eUSCI_B control word)
// UCMODE (Clock controls)
// UCB0BR0 (eUSCI_B bit rate)

hdev i2c_init(bus_instance_i2c bus, uint8_t slave_addr) {

    // Ensure USCI_B0 is reset before configuration
    UCB0CTL1 |= UCSWRST;

    // Set USCI_B0 to master mode I2C mode
    UCB0CTLW0 |= UCMST + UCMODE_3;

    // Configure the baud rate registers for 100kHz when sourcing from SMCLK
    // where SMCLK = 1MHz
    UCB0BR0 = 10;
    UCB0BR1 = 0;

    // automatic STOP assertion
    UCB0CTLW1 = UCASTP_2;

    // TX 7 bytes of data
    UCB0TBCNT = 0x07;

    // Address of slave is 12 hex
    UCB0I2CSA = slave_addr;

    // eUSCI_B in operational state
    UCB0CTL1 &= ^UCSWRST;

    // Enable TX-interrupt
    UCB0IE |= UCTXIE;

    // General interrupt enable
    GIE;

    // Fill TX buffer
    UCB0TXBUF = 0x77;

    // Take USCI_B0 out of reset and source clock from SMCLK
    UCB0CTL1 = UCSSEL_2;

    return 0;
}

static i2c_result i2cCoreRead(hDev device, uint8_t * buff, uint8_t szToRead, BOOL initialAutoStopSetup)
{
    uint8_t index = 0;

    bus_instance_i2c bus = devices[device].bus;
    I2CREG(bus, UCBxI2CSA) = devices[device].slaveaddr;

    // Setup autostop if havne't already done so
    if (initialAutoStopSetup)
    {
        i2cDisable(bus);
        i2cAutoStopSetTotalBytes(bus, szToRead);
        i2cEnable(bus);
    }

    // Send start message, in receive mode
    i2cMasterReceiveStart(bus);
    i2cWaitForStartComplete(bus);
    if(I2CREG(bus, UCBxCTLW0) & UCTXSTT) //error state
    {
        busErrorCount++;
        lastOperation = i2cRes_startTimeout;
        return i2cRes_startTimeout;
    }
    //  Stop bit will be auto-set once we read szToRead bytes
    while ( (I2CREG(bus, UCBxIFG) & UCSTPIFG) == 0 && index < szToRead)
    {
        if(I2CREG(bus, UCBxIFG) & UCNACKIFG)
        {
            //send a stop command
            i2cMasterTransmitStop(bus);
            i2cWaitForStopComplete(bus);
            if(I2CREG(bus, UCBxCTLW0) & UCTXSTP)
            {
                lastOperation = i2cRes_stopTimeout;
                return i2cRes_stopTimeout;
            }

            lastOperation = i2cRes_nack;
            return i2cRes_nack;
        }
        if ( (I2CREG(bus, UCBxIFG) & UCRXIFG) != 0)
            buff[index++] = i2cRetrieveReceiveBuffer(bus);
    }
    bytesRead += szToRead;
    lastOperation = i2cRes_noerror;
    return i2cRes_noerror;
}

static i2c_result i2cCoreWrite(hDev device, uint8_t * buff, uint8_t szToWrite, BOOL initialAutoStopSetup )
{
    uint8_t index = 0;

    bus_instance_i2c bus = devices[device].bus;
    I2CREG(bus, UCBxI2CSA) = devices[device].slaveaddr;

    if (initialAutoStopSetup)
    {
        i2cDisable(bus);
        i2cAutoStopSetTotalBytes(bus, szToWrite);
        i2cEnable(bus);
    }

    i2cMasterTransmitStart(bus);
    i2cWaitForStartComplete(bus);
    if(I2CREG(bus, UCBxCTLW0) & UCTXSTT) //error state
    {
        busErrorCount++;
        lastOperation = i2cRes_startTimeout;
        return i2cRes_startTimeout;
    }

    // Send in auto-stop or managed mode
    while ( (I2CREG(bus, UCBxIFG) & UCSTPIFG) == 0 && index < szToWrite)
    {
        if(I2CREG(bus, UCBxIFG) & UCNACKIFG)
        {
            //send a stop command
            i2cMasterTransmitStop(bus);
            i2cWaitForStopComplete(bus);
            if(I2CREG(bus, UCBxCTLW0) & UCTXSTP)
            {
                lastOperation = i2cRes_stopTimeout;
                return i2cRes_stopTimeout;
            }

            lastOperation = i2cRes_nack;
            return i2cRes_nack;
        }

        if ( (I2CREG(bus, UCBxIFG) & UCTXIFG0) != 0)
        {
            I2CREG(bus, UCBxTXBUF) = buff[index++];
        }
    }
    bytesWritten += szToWrite;
    lastOperation = i2cRes_noerror;
    return i2cRes_noerror;
}
