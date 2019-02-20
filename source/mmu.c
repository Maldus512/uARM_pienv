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
 * This module contains the initMMU function to enable the Memory Management Unit
 ******************************************************************************/

#include <stdint.h>
#include "arch.h"
#include "mmu.h"
#include "uart.h"
#include "asmlib.h"

#define NONGLOBAL 1

void init_page_tables(VMSAv8_64_NEXTLEVEL_DESCRIPTOR *level0, VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR *level1,
                      APBITS_TYPE permission) {
    uint32_t base = 0;

    // initialize 1:1 mapping for TTBR0
    // 1024MB of ram memory (some belongs to the VC)
    // default: 880 MB ARM ram, 128MB VC

    /* 880Mb of ram */
    for (base = 0; base < 440; base++) {
        // Each block descriptor (2 MB)
        level1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address   = (uintptr_t)base << (21 - 12),
                                                           .AF        = 1,
                                                           .nG        = NONGLOBAL,
                                                           .AP        = permission,
                                                           .SH        = SH_INNER_SHAREABLE,
                                                           .MemAttr   = MT_NORMAL_INDEX,
                                                           .EntryType = 1};
    }

    /* VC ram up to 0x3F000000 */
    for (; base < 512 - 8; base++) {
        // Each block descriptor (2 MB)
        level1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)base << (21 - 12),
                                                           .AF      = 1,
                                                           .nG      = NONGLOBAL,
                                                           .AP      = permission,
                                                           //.AP        = APBITS_NO_LIMIT,
                                                           .MemAttr   = MT_NORMAL_NC_INDEX,
                                                           .EntryType = 1};
    }

    /* 16 MB peripherals at 0x3F000000 - 0x40000000*/
    for (; base < 512; base++) {
        // Each block descriptor (2 MB)
        level1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address   = (uintptr_t)base << (21 - 12),
                                                           .AF        = 1,
                                                           .nG        = NONGLOBAL,
                                                           .AP        = APBITS_NO_LIMIT,
                                                           .MemAttr   = MT_DEVICE_NGNRNE_INDEX,
                                                           .EntryType = 1};
    }

    // 2 MB for mailboxes at 0x40000000
    // shared device, never execute
    level1[512] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address   = (uintptr_t)512 << (21 - 12),
                                                      .AF        = 1,
                                                      .nG        = NONGLOBAL,
                                                      .AP        = APBITS_NO_LIMIT,
                                                      .MemAttr   = MT_DEVICE_NGNRNE_INDEX,
                                                      .EntryType = 1};

    // unused up to 0x7FFFFFFF
    for (base = 513; base < 1024; base++) {
        level1[base].Raw64 = 0;
    }

    // All the rest of L2 entries are empty
    for (uint_fast16_t i = 0; i < 512; i++) {
        level0[i].Raw64 = 0;
    }

    // Just 2 valid entries mapping the 2GB in stage2
    level0[0] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR){.NSTable = 1, .Address = (uintptr_t)&level1[0] >> 12, .EntryType = 3};
    level0[1] =
        (VMSAv8_64_NEXTLEVEL_DESCRIPTOR){.NSTable = 1, .Address = (uintptr_t)&level1[512] >> 12, .EntryType = 3};
}



/**
 * Use the previous setup page translation tables
 */
void initMMU(uint64_t *page_table) {
    uint64_t r;
    uint64_t ttbr0;

    /* Set the memattrs values into mair_el1*/
    asm volatile("dsb sy");
    r = ((MT_DEVICE_NGNRNE << (MT_DEVICE_NGNRNE_INDEX * 8)) | (MT_DEVICE_NGNRE << (MT_DEVICE_NGNRE_INDEX * 8)) |
         (MT_DEVICE_GRE << (MT_DEVICE_GRE_INDEX * 8)) | (MT_NORMAL_NC << (MT_NORMAL_NC_INDEX * 8)) |
         (MT_NORMAL << (MT_NORMAL_INDEX * 8)));
    asm volatile("msr mair_el1, %0" : : "r"(r));

    ttbr0 = (uint64_t)&page_table[0];

    // Bring both tables online and execute memory barrier
    asm volatile("msr ttbr0_el1, %0" : : "r"((uintptr_t)ttbr0));
    asm volatile("msr ttbr1_el1, %0" : : "r"((uintptr_t)ttbr0));
    asm volatile("isb");

    /* Specify mapping characteristics in translate control register
        page D10-2685 of the ARM ARM */
    r = (0b00LL << 37) |      // TBI=0, no tagging
        (0b1LL << 36) |       // AS = 1, ASID 16 bits
        (0b000LL << 32) |     // IPS= 32 bit ... 000 = 32bit, 001 = 36bit, 010 = 40bit
        (0b10LL << 30) |      // TG1=4k ... options are 10=4KB, 01=16KB, 11=64KB ... take care differs from TG0
        (0b11LL << 28) |      // SH1=3 inner ... options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 =
                              // Inner Shareable
        (0b01LL << 26) |      // ORGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write
                              // thru cacheable, 11 = Write Back Non-cacheable
        (0b01LL << 24) |      // IRGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write
                              // thru cacheable, 11 = Write Back Non-cacheable
        (0b0LL << 23) |       // EPD1 ... Translation table walk disable for translations using TTBR1_EL1  0 = walk, 1 =
                              // generate fault
        (0b0LL << 22) |       // A1, TTBR0 holds the ASID
        (25LL << 0) |         // T1SZ=25 (512G) ... The region size is 2 POWER (64-T1SZ) bytes
        (0b00LL << 14) |      // TG0=4k  ... options are 00=4KB, 01=64KB, 10=16KB,  ... take care differs from TG1
        (0b11LL << 12) |      // SH0=3 inner ... .. options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 =
                              // Inner Shareable
        (0b01LL << 10) |      // ORGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write
                              // thru cacheable, 11 = Write Back Non-cacheable
        (0b01LL << 8) |     // IRGN0=1 write b`ack .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write
                            // thru cacheable, 11 = Write Back Non-cacheable
        (0b0LL << 7) |      // EPD0  ... Translation table walk disable for translations using TTBR0_EL1  0 = walk, 1 =
                            // generate fault
        (25LL << 0);        // T0SZ=25 (512G)  ... The region size is 2 POWER (64-T0SZ) bytes
    asm volatile("msr tcr_el1, %0; isb" : : "r"(r));

    /* Toggle some bits in system control register to enable page translation
        page D10-2654 of the ARM ARM */
    asm volatile("isb; mrs %0, sctlr_el1" : "=r"(r));

    r |= 0xC00800;         // set mandatory reserved bits
    r &= ~((1 << 25) |     // clear EE, little endian translation tables
           (1 << 24) |     // clear E0E
           (1 << 23) |     // Clear SPAN (no Privileged Access Never on exceptions)
           (1 << 19) |     // clear WXN
           (1 << 12) |     // clear I, no instruction cache
           (1 << 4) |      // clear SA0
           (1 << 3) |      // clear SA
           (1 << 2) |      // clear C, no cache at all
           (1 << 1));      // clear A, no aligment check
    r |= (1 << 0);         // set M, enable MMU

    RIGVBAR();

    asm volatile("msr sctlr_el1, %0; isb" : : "r"(r));
}
