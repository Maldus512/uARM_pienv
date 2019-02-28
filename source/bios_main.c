/*
 * Hardware Abstraction Layer for Raspberry Pi 3
 *
 * Copyright (C) 2018 Mattia Maldini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/******************************************************************************
 * Entry point for the Hardware Abstraction Layer
 ******************************************************************************/

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
    unsigned int serial[2];
    char string[64];
    *((uint64_t *)INTERRUPT_HANDLER)   = 0;
    *((uint64_t *)SYNCHRONOUS_HANDLER) = 0;
    *((uint8_t *)INTERRUPT_MASK)       = 0xFF;
    uint8_t *interrupt_lines           = (uint8_t *)INTERRUPT_LINES;
    uint8_t *deviceinstalled           = (uint8_t *)DEVICE_INSTALLED;

    for (i = 0; i < IL_LINES; i++) {
        interrupt_lines[i] = 0;
        deviceinstalled[i] = 0;
    }

    init_uart1();
    init_uart0();
    init_IPI();
    lfb_init();
    if (sd_init() == SD_OK) {
        fat_getpartition();
        fat_listdirectory();
        init_emulated_tapes();
        init_emulated_disks();
    }
    init_emulated_printers();
    init_emulated_timers();

    serial_number(serial);
    strcpy(string, "Board serial number: ");
    LOG(INFO, string);

    strcpy(string, "System timers runs at ");
    itoa(GETARMCLKFRQ(), &string[strlen(string)], 10);
    strcpy(&string[strlen(string)], " Hz");
    LOG(INFO, string);

    strcpy(string, "CPU-GPU memory split: ");
    itoa(get_memory_split(), &string[strlen(string)], 16);
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
    char     string[128];
    state_t  state;
    uint64_t ttbr0;
    void (*LDST_MMU)(void *addr);
    state.exception_link_register = (uint64_t)echo;
    state.stack_pointer           = (uint64_t)0x2000000 + 0x4000;
    state.status_register         = 0x300;
    ttbr0                         = (uint64_t)&Level0map1to1_el0[0];
    ttbr0 |= (1UL << 48);
    //state.TTBR0 = ttbr0;

    itoa(GETEL(), string, 10);
    LOG(INFO, string);

    init_page_tables(Level0map1to1_el1, Level1map1to1_el1, APBITS_NO_EL0);
    init_page_tables(Level0map1to1_el0, Level1map1to1_el0, APBITS_NO_LIMIT);

    uart0_puts("Echoing everything\n");
    //initMMU((uint64_t*)Level0map1to1_el1);

    //LDST_MMU = (void (*)(void *))((uint64_t)&LDST | 0xFFFF000000000000);
    LDST(&state);
    return 0;
}

void idle() {
    while (1) {
        asm volatile("wfi");
    }
}


void bios_main() {
    state_t kernel;

    initSystem();
    CoreExecute(1, idle);
    CoreExecute(2, idle);
    CoreExecute(3, idle);

    SYSCALL(0,0,0,0);

    STST(&kernel);
    kernel.exception_link_register = (uint64_t)main;
    kernel.status_register = 0x385;

    LDST(&kernel);
}
