#include "asmlib.h"
#include "libuarm.h"
#include "libuarmv2.h"
#include "uARMtypes.h"
#include "interrupts.h"

extern uint64_t millisecondsSinceStart;

typedef struct _state64_t {
    uint64_t xreg[28];
    uint64_t fp;
    uint64_t lr;
    uint64_t elr;
} state64_t;

void LDST(void *addr) {
    LDST_EL0(addr);
}

unsigned int getTODLO() {
    return (unsigned int) millisecondsSinceStart;
}

unsigned int getTODHI() {
    return (unsigned int) (millisecondsSinceStart >> 32);
}

unsigned int setTIMER(unsigned int timer) {
    return setNextTimerInterrupt(timer);
}