#include <stdint.h>

#include "gpio.h"


void initGpio() {
    uint32_t* gpio;

    gpio = (uint32_t*)GPIO_BASE;

    //Activate gpio 4 as output
    gpio[0] |= (1 << 12);
}


void setGpio(int num) {
    if (num < 32) {
        REG(GPIO_SET0) = 1 << num;
    } else {
        REG(GPIO_SET1) = 1 << num;
    }
}


void clearGpio(int num) {
    if (num < 32) {
        REG(GPIO_CLR0) = 1 << num;
    } else {
        REG(GPIO_CLR1) = 1 << num;
    }
}