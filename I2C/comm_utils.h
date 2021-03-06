/*
 * comm_utils.h
 *
 *  Created on: February 2, 2020
 *      Author: Logan Ince
 */

#ifndef COMM_UTILS_H_
#define COMM_UTILS_H_
#include <stdint.h>

/* unsure if anything else would also be used by other communication protocols on MSP430 like UART or SPI */

// Generic handle for buses and devices
typedef uint8_t hBus;
typedef uint8_t hDev;

#define BOOL uint8_t
#define TRUE 1
#define FALSE 0

#endif /* CORE_UTILS_H_ */
