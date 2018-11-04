#include "asmlib.h"
#include "libuarm.h"
#include "libuarmv2.h"
#include "uARMtypes.h"
#include "interrupts.h"
#include "timers.h"

extern uint64_t millisecondsSinceStart;

typedef struct _state64_t {
    uint64_t xreg[28];
    uint64_t fp;
    uint64_t lr;
    uint64_t elr;
} state64_t;

void LDST(void *state) {
    int el = SYSCALL(SYS_GETCURRENTEL, 0,0,0);
    if (el == 0) {
        SYSCALL(SYS_LAUNCHSTATE, state,0,0);
    }
    else {
        LDST_EL0(state);
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
    //TODO: actually implement it
    return 0xFFFFFFFF;
}