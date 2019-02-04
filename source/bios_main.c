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
#include "emulated_printers.h"
#include "emulated_tapes.h"
#include "emulated_disks.h"
#include "utils.h"

extern unsigned char *_kernel_memory_end;

__attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR    Level0map1to1_el0[512]  = {0};
__attribute__((aligned(4096))) VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR Level1map1to1_el0[1024] = {0};

__attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR    Level0map1to1_el1[512]  = {0};
__attribute__((aligned(4096))) VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR Level1map1to1_el1[1024] = {0};

void initSystem() {
    int  i = 0;
    char string[64];
    *((uint64_t *)INTERRUPT_HANDLER)   = 0;
    *((uint64_t *)SYNCHRONOUS_HANDLER) = 0;
    *((uint8_t *)INTERRUPT_MASK)       = 0xFF;
    uint8_t *interrupt_lines           = (uint8_t *)INTERRUPT_LINES;

    for (i = 0; i < IL_LINES; i++) {
        interrupt_lines[i] = 0;
    }

    init_uart1();
    init_uart0();
    startUart0Int();
    initIPI();
    lfb_init();
    if (sd_init() == SD_OK) {
        fat_getpartition();
        fat_listdirectory();
        init_emulated_tapes();
        init_emulated_disks();
    }
    init_emulated_printers();
    init_emulated_timers();

    strcpy(string, "System timers runs at ");
    itoa(GETARMCLKFRQ(), &string[strlen(string)], 10);
    strcpy(&string[strlen(string)], " Hz");
    LOG(INFO, string);

    strcpy(string, "CPU-GPU memory split: ");
    itoa(getMemorySplit(), &string[strlen(string)], 16);
    LOG(INFO, string);
    strcpy(string, "Kernel occupies memory from 0x80000 to ");
    itoa((uint64_t)&_kernel_memory_end, &string[strlen(string)], 16);
    LOG(INFO, string);
    uart0_puts("\n");
    uart0_puts("************************************\n");
    uart0_puts("*        MaldOS running...         *\n");
    uart0_puts("************************************\n");
    uart0_puts("\n");

    init_arm_timer_interrupt();
}


void function1() {
    uart0_puts("ciao");
    while (1) {
        delay_us(500 * 1000);
        uart0_puts("ciao uart0\n");
    }
}

void idle2() {
    state_t state;
    state.exception_link_register = (uint64_t)function1;
    state.stack_pointer           = (uint64_t)0x1000000 + 0x2000;
    state.status_register         = 0x300;
    LDST(&state);
}

void echo() {
    while (1) {
        uart0_putc(uart0_getc());
        // GIC->Core0_MailBox0_WriteSet = 2;
    }
}

int __attribute__((weak)) main() {
    char string[128];
    state_t state;
    uint64_t ttbr0;
    void (*LDST_MMU)(void* addr);
    state.exception_link_register = (uint64_t)function1;
    state.stack_pointer           = (uint64_t)0x2000000 + 0x4000;
    state.status_register         = 0x300;
    ttbr0 = (uint64_t)&Level0map1to1_el0[0];
    ttbr0 |= (1UL << 48);
    state.TTBR0                   =ttbr0;

    itoa(ttbr0, string, 16);
    LOG(INFO, string);

    uart0_puts("Echoing everything\n");
    LDST_MMU = ((uint64_t)&LDST | 0xFFFF000000000000);
    LDST_MMU(&state);
    while (1) {
        uart0_putc(uart0_getc());
    }
}

void idle() {
    while (1) {
        asm volatile("wfi");
    }
}


void bios_main() {
    initSystem();
    CoreExecute(1, idle);
    CoreExecute(2, idle);
    CoreExecute(3, idle);

    init_page_tables(Level0map1to1_el1, Level1map1to1_el1, APBITS_NO_EL0);
    init_page_tables(Level0map1to1_el0, Level1map1to1_el0, APBITS_NO_LIMIT);
    //initMMU((uint64_t*)&Level0map1to1_el1[0]);

    asm volatile("msr daifset, #2");
    main();
}
