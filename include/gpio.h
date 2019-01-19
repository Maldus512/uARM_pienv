#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>
#include "arch.h"


struct GPIOREG {
    volatile uint32_t SEL[6];
    volatile uint32_t rsvd0;
    volatile uint32_t SET0;
    volatile uint32_t SET1;
    volatile uint32_t rsvd1;
    volatile uint32_t CLEAR0;
    volatile uint32_t CLEAR1;
    volatile uint32_t rsvd2;
    volatile uint32_t LVL0;
    volatile uint32_t LVL1;
    volatile uint32_t rsvd3;
    volatile uint32_t EVENT0;
    volatile uint32_t EVENT1;
    volatile uint32_t rsvd4;
    volatile uint32_t REDETECTEN0;
    volatile uint32_t REDETECTEN1;
    volatile uint32_t rsvd5;
    volatile uint32_t FEDETECTEN0;
    volatile uint32_t FEDETECTEN1;
    volatile uint32_t rsvd6;
    volatile uint32_t HIDETECTEN0;
    volatile uint32_t HIDETECTEN1;
    volatile uint32_t rsvd7;
    volatile uint32_t LODETECTEN0;
    volatile uint32_t LODETECTEN1;
    volatile uint32_t rsvd8;
    volatile uint32_t AREDETECTEN0;
    volatile uint32_t AREDETECTEN1;
    volatile uint32_t rsvd9;
    volatile uint32_t AFEDETECTEN0;
    volatile uint32_t AFEDETECTEN1;
    volatile uint32_t rsvd10;
    volatile uint32_t PUD;
    volatile uint32_t PUDCLOCK0;
    volatile uint32_t PUDCLOCK1;
};

#define GPIO_BASE (IO_BASE + 0x200000)

#define GPIO ((struct GPIOREG *)GPIO_BASE)

#define LED_RUN 21


typedef enum {
    GPIO_INPUT    = 0x00,
    GPIO_OUTPUT   = 0x01,
    GPIO_ALTFUNC5 = 0x02,
    GPIO_ALTFUNC4 = 0x03,
    GPIO_ALTFUNC0 = 0x04,
    GPIO_ALTFUNC1 = 0x05,
    GPIO_ALTFUNC2 = 0x06,
    GPIO_ALTFUNC3 = 0x07,
} GPIOMODE;

typedef enum {
    GPIO_PUD_DISABLE = 0x0,
    GPIO_PULL_DOWN   = 0x1,
    GPIO_PULL_UP     = 0x2,
} GPIOPUD;


void setupGpio(uint_fast8_t num, GPIOMODE mode);
void setPullUpDown(unsigned int gpio, GPIOPUD mode);
void setHighDetect(unsigned int gpio);

void setGpio(int num);
void clearGpio(int num);

#endif