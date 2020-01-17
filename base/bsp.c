#include "bsp.h"

void bspI2CInit() {
   // Stop watchdog timer
   WDTCTL = WDTPW | WDTHOLD;
   
   // Disable the GPIO power-on default high-impedance mode to activate
   // previously configured port settings
   PM5CTL0 &= ~LOCKLPM5;
}