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
 * This is a module with miscellaneous utility functions
 ******************************************************************************/

#include "arch.h"

void memcpy(void *dest, void *src, int n) {
    // Typecast src and dest addresses to (char *)
    char *csrc  = (char *)src;
    char *cdest = (char *)dest;

    // Copy contents of src[] to dest[]
    for (int i = 0; i < n; i++)
        cdest[i] = csrc[i];
}

int memcmp(unsigned char *str1, unsigned char *str2, int count) {
    register const unsigned char *s1 = (const unsigned char *)str1;
    register const unsigned char *s2 = (const unsigned char *)str2;

    while (count-- > 0) {
        if (*s1++ != *s2++)
            return s1[-1] < s2[-1] ? -1 : 1;
    }
    return 0;
}

char *itoa(long unsigned int value, char *result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) {
        *result = '\0';
        return result;
    }

    if (base == 16) {
        *result++ = '0';
        *result++ = 'x';
    }

    char *            ptr = result, *ptr1 = result, tmp_char;
    long unsigned int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ =
            "ZYXWVUTSRQPONMLKJIHGFEDCBA9876543210123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[35 + (tmp_value - value * base)];
    } while (value);

    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--   = *ptr1;
        *ptr1++  = tmp_char;
    }
    return result;
}

int strlen(char *str) {
    int len = 0;
    while (str[len] != '\0')
        len++;
    return len;
}

int strcpy(char *dest, char *src) {
    int len = 0;
    while (src[len] != '\0') {
        dest[len] = src[len];
        len++;
    }
    dest[len] = src[len];
    return len;
}

void *memset(void *s, int c, unsigned int n) {
    unsigned char *p = s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}