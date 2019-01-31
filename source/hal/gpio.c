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
 * This module is a GPIO library
 ******************************************************************************/

#include <stdint.h>
#include "gpio.h"

/*
 * Initialize a GPIO as input, output or alternate mode
 */
void setupGpio(uint_fast8_t gpio, GPIOMODE mode) {
    int reg = gpio / 10;
    if (gpio > 54)
        return;     // Check GPIO pin number valid, return false if invalid
    if (mode < 0 || mode > GPIO_ALTFUNC3)
        return;                                // Check requested mode is valid, return false if invalid
    uint_fast32_t bit = ((gpio % 10) * 3);     // Create bit mask
    uint32_t      mem = GPIO->SEL[reg];        // Read register
    mem &= ~(7 << bit);                        // Clear GPIO mode bits for that port
    mem |= (mode << bit);                      // Logical OR GPIO mode bits
    GPIO->SEL[reg] = mem;                      // Write value to register
}

/*
 * Initialize internal pull up or pull down for a GPIO
 */
void setPullUpDown(unsigned int gpio, GPIOPUD mode) {
    int c = 0;
    GPIO->PUD = mode;

    while(c++ <= 150)
        nop();

    if (gpio < 32)
        GPIO->PUDCLOCK0 = (1 << gpio);
    else
        GPIO->PUDCLOCK1 = (1 << (32-gpio));

    while(c++ <= 150)
        nop();

    GPIO->PUD = 0;
    GPIO->PUDCLOCK0 = 0;
    GPIO->PUDCLOCK1 = 0;
}

/*
 * Enable high detect for a GPIO
 */
void setHighDetect(unsigned int gpio) {
    if (gpio < 32)
        GPIO->HIDETECTEN0 = (1 << gpio);
    else
        GPIO->HIDETECTEN1 = (1 << (32-gpio));
}


/*
 * Set GPIO level as high
 */
void setGpio(int num) {
    if (num < 32) {
        GPIO->SET0 = 1 << num;
    } else if (num > 0) {
        GPIO->SET1 = 1 << num;
    }
}


/*
 * Set GPIO level as low
 */
void clearGpio(int num) {
    if (num < 32) {
        GPIO->CLEAR0 = 1 << num;
    } else if (num > 0) {
        GPIO->CLEAR1 = 1 << num;
    }
}
