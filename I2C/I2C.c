/*
 * I2C.c
 *
 *  Created on: Jan 16, 2020
 *      Author: Logan Ince
 */

/*
 * Lots of the below is copy pasted from jeffc's i2c.c
 * Changing what should be changed to facilitate better legibility for future,
 * primarily through commenting, though will refactor if necessary.
 */

#include "I2C.h"

// defines each field and its "get" method individually
uint16_t busErrorCount = 0;
uint16_t i2cGetBusErrorCount()
{
    return busErrorCount;
}

uint16_t bytesRead = 0;
uint16_t i2cGetBytesRead()
{
    return bytesRead;
}

uint16_t bytesWritten = 0;
uint16_t i2cGetBytesWritten()
{
    return bytesWritten;
}

i2c_result lastOperation = i2cRes_noerror;
i2c_result i2cGetLastOperationResult()
{
    return lastOperation;
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

/* How can we avoid the unpleasantness? Is this even necessary? */
// Some quasi-unpleasant hackery to make for clean dual-bus support in general
static void populateBusRegisters(bus_instance_i2c instance)
{
    bus_registers_i2c * pRegs;

    if (instance == I2CBus1)
    {
        pRegs = &busregs[1];
        pRegs->UCBxCTLW0 =  &UCB1CTLW0;
        pRegs->UCBxCTL1 =   &UCB1CTL1;   // MSB of CTLW0
        pRegs->UCBxCTLW1 =  &UCB1CTLW1;
        pRegs->UCBxBRW =    &UCB1BRW;
        pRegs->UCBxSTATW =  &UCB1STATW;
        pRegs->UCBxTBCNT =  &UCB1TBCNT;
        pRegs->UCBxRXBUF =  &UCB1RXBUF;
        pRegs->UCBxTXBUF =  &UCB1TXBUF;
        pRegs->UCBxI2CSA =  &UCB1I2CSA;
        pRegs->UCBxIE =     &UCB1IE;
        pRegs->UCBxIFG =    &UCB1IFG;
        pRegs->UCBxIV =     &UCB1IV;
    }
    else if (instance == I2CBus2)
    {
        pRegs = &busregs[2];
        pRegs->UCBxCTLW0 =  &UCB2CTLW0;
        pRegs->UCBxCTL1 =   &UCB2CTL1;   // MSB of CTLW0
        pRegs->UCBxCTLW1 =  &UCB2CTLW1;
        pRegs->UCBxBRW =    &UCB2BRW;
        pRegs->UCBxSTATW =  &UCB2STATW;
        pRegs->UCBxTBCNT =  &UCB2TBCNT;
        pRegs->UCBxRXBUF =  &UCB2RXBUF;
        pRegs->UCBxTXBUF =  &UCB2TXBUF;
        pRegs->UCBxI2CSA =  &UCB2I2CSA;
        pRegs->UCBxIE =     &UCB2IE;
        pRegs->UCBxIFG =    &UCB2IFG;
        pRegs->UCBxIV =     &UCB2IV;
    }
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

2c_result i2cMasterRead(hDev device, uint8_t * buff, uint8_t szToRead)
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

/*  I understand nothing about these vector functions */
// Primary interrupt vector for I2C on module B1 (I2C #1)
#pragma vector = EUSCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void)
{
    switch(__even_in_range(UCB1IV, USCI_I2C_UCBIT9IFG))
    {
        case USCI_NONE:          break;     // Vector 0: No interrupts
        case USCI_I2C_UCALIFG:   break;     // Vector 2: ALIFG
        case USCI_I2C_UCNACKIFG:            // Vector 4: NACKIFG
            UCB1CTL1 |= UCTXSTT;            // Received NACK ... try again, start
            break;
        case USCI_I2C_UCSTTIFG:  break;     // Vector 6: STTIFG
        case USCI_I2C_UCSTPIFG:  break;     // Vector 8: STPIFG
        case USCI_I2C_UCRXIFG3:  break;     // Vector 10: RXIFG3
        case USCI_I2C_UCTXIFG3:  break;     // Vector 12: TXIFG3
        case USCI_I2C_UCRXIFG2:  break;     // Vector 14: RXIFG2
        case USCI_I2C_UCTXIFG2:  break;     // Vector 16: TXIFG2
        case USCI_I2C_UCRXIFG1:  break;     // Vector 18: RXIFG1
        case USCI_I2C_UCTXIFG1:  break;     // Vector 20: TXIFG1
        case USCI_I2C_UCRXIFG0:  break;     // Vector 22: RXIFG0
        case USCI_I2C_UCTXIFG0:  break;     // Vector 24: TXIFG0
        case USCI_I2C_UCBCNTIFG: break;     // Vector 26: BCNTIFG
        case USCI_I2C_UCCLTOIFG: break;     // Vector 28: clock low timeout
        case USCI_I2C_UCBIT9IFG: break;     // Vector 30: 9th bit
        default: break;
    }
}

// Primary interrupt vector for I2C on module B2 (I2C #2)
#pragma vector = EUSCI_B2_VECTOR
__interrupt void USCI_B2_ISR(void)
{
    switch(__even_in_range(UCB2IV, USCI_I2C_UCBIT9IFG))
    {
        case USCI_NONE:          break;     // Vector 0: No interrupts
        case USCI_I2C_UCALIFG:   break;     // Vector 2: ALIFG
        case USCI_I2C_UCNACKIFG:            // Vector 4: NACKIFG
            UCB2CTL1 |= UCTXSTT;            // Received NACK ... try again, start
            break;
        case USCI_I2C_UCSTTIFG:  break;     // Vector 6: STTIFG
        case USCI_I2C_UCSTPIFG:  break;     // Vector 8: STPIFG
        case USCI_I2C_UCRXIFG3:  break;     // Vector 10: RXIFG3
        case USCI_I2C_UCTXIFG3:  break;     // Vector 12: TXIFG3
        case USCI_I2C_UCRXIFG2:  break;     // Vector 14: RXIFG2
        case USCI_I2C_UCTXIFG2:  break;     // Vector 16: TXIFG2
        case USCI_I2C_UCRXIFG1:  break;     // Vector 18: RXIFG1
        case USCI_I2C_UCTXIFG1:  break;     // Vector 20: TXIFG1
        case USCI_I2C_UCRXIFG0:  break;     // Vector 22: RXIFG0
        case USCI_I2C_UCTXIFG0:  break;     // Vector 24: TXIFG0
        case USCI_I2C_UCBCNTIFG: break;     // Vector 26: BCNTIFG
        case USCI_I2C_UCCLTOIFG: break;     // Vector 28: clock low timeout
        case USCI_I2C_UCBIT9IFG: break;     // Vector 30: 9th bit
        default: break;
    }
}
