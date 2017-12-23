#include  "hardwareprofile.h"

extern int __bss_start;
extern int __bss_end;

extern void bios_main( uint32_t r0, uint32_t r1, uint32_t atags );

void _crt0( uint32_t r0, uint32_t r1, uint32_t r2 ) {
    /*__bss_start and __bss_end are defined in the linker script */
    int* bss = &__bss_start;
    int* bss_end = &__bss_end;

    while( bss < bss_end ) {
        *bss++ = 0;
    }

    bios_main( r0, r1, r2 );

    while(1) {
        nop();
    }
}