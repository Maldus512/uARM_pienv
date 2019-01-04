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


void initSystem() {
    *((uint64_t *)INTERRUPT_HANDLER)   = 0;
    *((uint64_t *)SYNCHRONOUS_HANDLER) = 0;
    *((uint8_t *)INTERRUPT_MASK)       = 0xFF;

    initGpio();
    initUart0();
    startUart0Int();
    lfb_init();
    if (sd_init() == SD_OK) {
        fat_getpartition();
        init_emulated_tapes();
    }
    init_emulated_terminals();
    uart0_puts("CPU-GPU memory split: ");
    hexstring(getMemorySplit());
    uart0_puts("\n");
    uart0_puts("************************************\n");
    uart0_puts("*        MaldOS running...         *\n");
    uart0_puts("************************************\n");
    uart0_puts("\n");

    lfb_print(1, 1, "*******************************");
    lfb_print(1, 2, "*      MaldOS running...      *");
    lfb_print(1, 3, "*******************************");

    initArmTimer();
    setTimer(1);
}


void function1() {
    uart0_puts("ciao");
    while(1) {
        uart0_puts("ciao\n");
        delay_us(1000*1000);
    }
}

void function2() {
    while(1) {
        uart0_puts("ciao numero 2\n");
        delay_us(800*1000);
    }
}

void idle() {
    state_t state;
    state.exception_link_register = (uint64_t)function1;
    state.stack_pointer           = (uint64_t)0x1000000 + 0x2000;
    state.status_register         = 0x340;
    setTimer(1000*1000);
    LDST(&state);
}

void idle2() {
    state_t state;
    state.exception_link_register = (uint64_t)function2;
    state.stack_pointer           = (uint64_t)0x1000000 + 0x4000;
    state.status_register         = 0x340;
    setTimer(500*1000);
    LDST(&state);
}




int __attribute__((weak)) main() {
    uart0_puts("Echoing everything\n");
    // echo everything back
    CoreExecute(1, idle);
    idle2();
    while (1) {
        uart0_putc(uart0_getc());
    }
}


void bios_main() {
    initSystem();
    // init_page_table();
    // mmu_init();
    main();
}
