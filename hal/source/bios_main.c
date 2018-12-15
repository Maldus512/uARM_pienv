#include "arch.h"
#include "bios_const.h"
#include "uart.h"
#include "gpio.h"
#include "mailbox.h"
#include "asmlib.h"
#include "timers.h"
#include "libuarm.h"
#include "interrupts.h"
#include "mmu.h"
#include "sd.h"
#include "system.h"
#include "fat.h"
#include "emulated_terminals.h"
#include "emulated_tapes.h"

#ifdef APP
extern void main();
#endif

uint64_t ciao[256];


extern int __uMPS_stack;

void initSystem() {
    *((uint64_t *)INTERRUPT_HANDLER)   = 0;
    *((uint64_t *)SYNCHRONOUS_HANDLER) = 0;
    *((uint8_t *)INTERRUPT_MASK) = 0xFF;

    initGpio();
    initUart0();
    lfb_init();
    if (sd_init() == SD_OK) {
        fat_getpartition();
        init_emulated_tapes();
    }
    init_emulated_terminals();
    uart0_puts("\n");
    uart0_puts("************************************\n");
    uart0_puts("*        MaldOS running...         *\n");
    uart0_puts("************************************\n");
    uart0_puts("\n");

    lfb_print(1, 1, "*******************************");
    lfb_print(1, 2, "*      MaldOS running...      *");
    lfb_print(1, 3, "*******************************");

    SYSCALL(SYS_INITARMTIMER, 0, 0, 0);
    SYSCALL(SYS_SETNEXTTIMER, 1, 0, 0);
}
void mydelay(unsigned int us) {
    //delay_us(us); return;
    volatile unsigned long timestamp = get_us();
    volatile unsigned long end = timestamp+us;
    while((unsigned long)timestamp < (unsigned long)(end )) {
        timestamp = get_us();
        /*hexstrings(timestamp);
        tprint(" - ");
        hexstrings(start+us);
        tprint("\n");*/
        nop();
    }
}


void bios_main() {
    initSystem();


#ifdef APP
    main();
#endif

    // echo everything back
    while (1) {
        uart0_putc(uart0_getc());
    }
}
