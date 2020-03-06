#include <stdint.h>
#include <msp430.h>

void bspInit();
void bspI2CInit( bus_instance_i2c instance);
void bspSPIInit( bus_instance_spi instance);