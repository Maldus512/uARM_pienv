#include <stdint.h>

#include "gpio.h"


void initGpio() {
    // Activate gpio 4 as output
    setupGpio(LED_RUN, GPIO_OUTPUT);
}

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

void setHighDetect(unsigned int gpio) {
    if (gpio < 32)
        GPIO->HIDETECTEN0 = (1 << gpio);
    else
        GPIO->HIDETECTEN1 = (1 << (32-gpio));
}


void setGpio(int num) {
    if (num < 32) {
        GPIO->SET0 = 1 << num;
    } else if (num > 0) {
        GPIO->SET1 = 1 << num;
    }
}


void clearGpio(int num) {
    if (num < 32) {
        GPIO->CLEAR0 = 1 << num;
    } else if (num > 0) {
        GPIO->CLEAR1 = 1 << num;
    }
}
