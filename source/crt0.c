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
 * This module contains the C runtime initialization
 ******************************************************************************/

/*__bss_start and __bss_end are defined in the linker script */
extern int __bss_start;
extern int __bss_end;

extern void bios_main();

/* Initializer of the C runtime, i.e. function that zeros the bss section */
void _crt0() {
    int *bss     = &__bss_start;
    int *bss_end = &__bss_end;

    while (bss < bss_end) {
        *bss++ = 0;
    }

    bios_main();

    // Endless loop, should never run
    while (1) {
        asm volatile("nop");
    }
}