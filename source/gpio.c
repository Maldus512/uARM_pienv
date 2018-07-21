#include <stdint.h>

#include "gpio.h"


void initGpio() {
    //Activate gpio 47 as output
    GPIO->SEL[4] = 0;
    GPIO->SEL[4] |= (1 << 21);

}


void setGpio(int num) {
    if (num < 32) {
        GPIO->SET0 = 1 << num;
    } else if (num > 0) {
        GPIO->SET1 = 1 << (num-32);
    }
}


void clearGpio(int num) {
    if (num < 32) {
        GPIO->CLEAR0 = 1 << num;
    } else if (num > 0){
        GPIO->CLEAR1 = 1 << (num-32);
    }
}
