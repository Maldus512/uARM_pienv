#include "system.h"
#include "asmlib.h"
#include "libuarm.h"
#include "interrupts.h"
#include "timers.h"

//extern uint64_t millisecondsSinceStart;

void LDST(void *state) {
    //TODO: prevent loading EL1 from EL0
    int el = SYSCALL(SYS_GETCURRENTEL, 0,0,0);
    if (el == 0) {
        SYSCALL(SYS_LAUNCHSTATE, (long unsigned int)state,0,0);
    }
    else {
        LDST_EL0(state);
    }
}

void STST(void *state) {
    int el = SYSCALL(SYS_GETCURRENTEL, 0,0,0);
    STST_EL0(state);
    ((state_t*)state)->status_register = SYSCALL(SYS_GETCURRENTSTATUS,0,0,0);
    if (el == 0) {
        ((state_t*)state)->TTBR0 = SYSCALL(SYS_GETTTBR0,0,0,0);
    }
}

unsigned int getTODLO() {
    unsigned int tmp;
    uint32_t freq = GETARMCLKFRQ();
    tmp = (unsigned int)(readCounterCount()/(freq/1000));
    return tmp;
}

unsigned int getTODHI() {
    unsigned int tmp;
    uint32_t freq = GETARMCLKFRQ();
    tmp = (unsigned int)((readCounterCount()/(freq/100)) >> 32);
    return tmp;
}

unsigned int setTIMER(unsigned int timer) {
    return SYSCALL(SYS_SETNEXTTIMER, timer, 0,0);
}

unsigned int getCAUSE() {
    uint64_t cause = 0, tmp;

    tmp = *((volatile uint32_t *)CORE0_IRQ_SOURCE); 

    if (tmp & (1 << 3)) {
        cause |= TIMER_INT_LINE;
    }

    if (tmp & (1 << 8)) {
        cause |= UART0_INT_LINE;
    }

    return cause;
}

