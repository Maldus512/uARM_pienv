//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

extern void PUT32 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
extern void dummy ( unsigned int );

#include "hardwareprofile.h"
#include "gpio.h"
#include "timers.h"



int bios_main(uint32_t r1, uint32_t r2, uint32_t atags) {
    initGpio();

    while(1) {
        setGpio(47);
        rawDelay();
        clearGpio(47);
        rawDelay();
    }
}