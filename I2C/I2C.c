/*
 * I2C.c
 *
 *  Created on: Jan 16, 2020
 *      Author: Logan Ince
 */

#include "I2C.h";



typedef uint8_t hDev;

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

i2c_result i2cMasterRead(hDev device, uint8_t * buff, uint8_t szToRead)
{
    return i2cCoreRead(device, buff, szToRead, TRUE);
}
i2c_result i2cMasterWrite(hDev device, uint8_t * buff, uint8_t szToWrite)
{
    return i2cCoreWrite(device, buff, szToWrite, TRUE);
}

i2c_result i2cMasterCombinedWriteRead(hDev device, uint8_t * wbuff, uint8_t szToWrite, uint8_t * rbuff, uint8_t szToRead)
{
    bus_instance_i2c bus = devices[device].bus;

    i2cWaitForStopComplete(bus);

    i2cDisable(bus);
    i2cAutoStopSetTotalBytes(bus, szToWrite + szToRead);
    i2cEnable(bus);
    lastOperation = i2cCoreWrite(device, wbuff, szToWrite, FALSE);
    if(lastOperation)
        return lastOperation;

    i2cWaitReadyToTransmitByte(bus);
    if((I2CREG(bus, UCBxIFG) & UCTXIFG0) == 0)
    {
        lastOperation = i2cRes_transmitTimeout;
        return i2cRes_transmitTimeout;
    }
    // CoreRead sets the last operation
    return i2cCoreRead(device, rbuff, szToRead, FALSE);
}

i2c_result i2cMasterRegisterRead(hDev device, uint8_t registeraddr, uint8_t * buff, uint8_t szToRead)
{
    bus_instance_i2c bus = devices[device].bus;

    i2cWaitForStopComplete(bus);
    if(I2CREG(bus, UCBxCTLW0) & UCTXSTP)
    {
        lastOperation = i2cRes_stopTimeout;
        return i2cRes_stopTimeout;
    }

    return i2cMasterCombinedWriteRead(device, &registeraddr, 1, buff, szToRead);
}

void i2cReset()
{
    // the I2C bus numbering is 1-indexed
    uint8_t i;
    for (i = 1; i < CONFIGM_i2c_maxperipheralinstances; i++)
    {
        if (buses[i].initialized)
        {
            bus_instance_i2c bus = (bus_instance_i2c)i;
            i2cDisable(bus);
            bspI2CInit(bus);
            i2cEnable(bus);
        }
    }
    busErrorCount = 0;
}

hDev i2cInit(bus_instance_i2c bus, uint8_t slaveaddr)
{
    bus_context_i2c * pBus = &(buses[(uint8_t)bus]);

    // First handle bus initialization, if necessary
    if (!pBus->initialized)
    {
        pBus->initialized = TRUE;

        bspI2CInit(bus);
        populateBusRegisters(bus);

        // USCI Configuration:  Common for both TX and RX
        i2cDisable(bus);                                             // Software reset enabled
        I2CREG(bus, UCBxCTLW0) |= UCMODE_3 | UCMST | UCSYNC | UCSSEL__SMCLK;     // I2C, master, synchronous, use SMCLK
        I2CREG(bus, UCBxBRW) = 12;                                               // Baudrate = SMCLK / 12
        I2CREG(bus, UCBxCTLW1) |= UCASTP_2;                                      // Automatic stop generated after transmission complete
        I2CREG(bus, UCBxCTLW1) |= UCCLTO_3;                                      // clk low timeout @ 34ms
        i2cEnable(bus);                                             // Software reset disabled
    }

    // Now setup the actual individual device
    uint8_t currindex = numDevices;
    numDevices++;
    pBus->num_devices++;

    devices[currindex].bus = bus;
    devices[currindex].slaveaddr = slaveaddr;

    return (hDev)currindex;
}
