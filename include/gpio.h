#ifndef __GPIO_H__
#define __GPIO_H__

#include "hardwareprofile.h"

//TODO: access registers through structures

struct GPIOREG {
    uint32_t SEL[6];
    uint32_t rsvd0;
    uint32_t SET0;
    uint32_t SET1;
    uint32_t rsvd1;
    uint32_t CLEAR0;
    uint32_t CLEAR1;
    uint32_t rsvd2;
    uint32_t LVL0;
    uint32_t LVL1;
    uint32_t rsvd3;
    uint32_t EVENT0;
    uint32_t EVENT1;
    uint32_t rsvd4;
    uint32_t REDETECTEN0;
    uint32_t REDETECTEN1;
    uint32_t rsvd5;
    uint32_t FEDETECTEN0;
    uint32_t FEDETECTEN1;
    uint32_t rsvd6;
    uint32_t HIDETECTEN0;
    uint32_t HIDETECTEN1;
    uint32_t rsvd7;
    uint32_t LODETECTEN0;
    uint32_t LODETECTEN1;
    uint32_t rsvd8;
    uint32_t AREDETECTEN0;
    uint32_t AREDETECTEN1;
    uint32_t rsvd9;
    uint32_t AFEDETECTEN0;
    uint32_t AFEDETECTEN1;
    uint32_t rsvd10;
    uint32_t PUDEN;
    uint32_t PUDCLOCK0;
    uint32_t PUDCLOCK1;
};

#define GPIO_BASE       (IO_BASE + 0x200000)

#define GPIO            ((struct GPIOREG*) GPIO_BASE)


void initGpio();

void setGpio(int num);
void clearGpio(int num);

#endif