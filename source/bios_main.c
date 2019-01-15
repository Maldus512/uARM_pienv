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
#include "utils.h"


void initSystem() {
    int  i = 0;
    char string[32];
    *((uint64_t *)INTERRUPT_HANDLER)   = 0;
    *((uint64_t *)SYNCHRONOUS_HANDLER) = 0;
    *((uint8_t *)INTERRUPT_MASK)       = 0xFF;
    uint8_t *interrupt_lines           = (uint8_t *)INTERRUPT_LINES;

    for (i = 0; i < IL_LINES; i++) {
        interrupt_lines[i] = 0;
    }

    initGpio();
    initUart1();
    initUart0();
    startUart0Int();
    initIPI();
    lfb_init();
    if (sd_init() == SD_OK) {
        fat_getpartition();
        fat_listdirectory();
        init_emulated_tapes();
    }
    init_emulated_terminals();
    init_emulated_timers();
    strcpy(string, "CPU-GPU memory split: ");
    itoa(getMemorySplit(), &string[strlen(string)], 16);
    LOG(INFO, string);
    uart0_puts("\n");
    uart0_puts("************************************\n");
    uart0_puts("*        MaldOS running...         *\n");
    uart0_puts("************************************\n");
    uart0_puts("\n");

    lfb_print(1, 1, "*******************************");
    lfb_print(1, 2, "*      MaldOS running...      *");
    lfb_print(1, 3, "*******************************");

    initArmTimer();
    // setTimer(1);
}


void function1() {
    uart0_puts("ciao");
    while (1) {
        initUart1();
        uart1_puts("ciao uart1\n");
        delay_us(500 * 1000);
        initUart0();
        uart0_puts("ciao uart0\n");
        delay_us(500 * 1000);
    }
}

void idle() {
    state_t state;
    state.exception_link_register = (uint64_t)function1;
    state.stack_pointer           = (uint64_t)0x1000000 + 0x2000;
    state.status_register         = 0x300;
    setTimer(1000 * 1000);
    LDST(&state);
}

void echo() {
    while (1) {
        uart0_putc(uart0_getc());
        // GIC->Core0_MailBox0_WriteSet = 2;
    }
}

int __attribute__((weak)) main() {
    /*state_t state;
    state.exception_link_register = (uint64_t)echo;
    state.stack_pointer           = (uint64_t)0x1000000 + 0x4000;
    state.status_register         = 0x300;
    //setTimer(1000*1000);*/
    uart0_puts("Echoing everything\n");
    CoreExecute(1, idle);
    // LDST(&state);
    while (1) {
        uart0_putc(uart0_getc());
        GIC->Core0_MailBox0_WriteSet = 2;
    }
}


void bios_main() {
    char         buffer[101];
    unsigned int licence;
    unsigned int read = 0;
    initSystem();

    licence = fat_getcluster("BROADCOM   ");
    while (read < 0xC8C) {
        read += fat_readfile(licence, buffer, read, 100);
        buffer[100] = '\0';
        uart0_puts(buffer);
    }
    /*init_page_table();
    mmu_init();
    vbarForMMU();*/

    main();
}
