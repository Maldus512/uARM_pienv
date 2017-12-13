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
        GPIO->SET0 = 1 << num;
    } else {
        GPIO->SET1 = 1 << num;
    }
}


void clearGpio(int num) {
    if (num < 32) {
        GPIO->CLEAR0 = 1 << num;
    } else {
        GPIO->CLEAR1 = 1 << num;
    }
}