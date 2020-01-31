#include <msp430.h>

extern int BAUDRATE = 0x008; /* declared in bsp.c */
extern int TX = 0x07; /* declared in bsp.c */
extern int SLVADD = 0x0012; /* declared in bsp.c */
extern int I2CPN = 0x03; /* declared in bsp.c */
extern int TXBFFR = 0x77; /* declared in bsp.c */

void bspI2CInit();