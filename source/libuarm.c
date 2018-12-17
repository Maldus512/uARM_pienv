#include "system.h"
#include "asmlib.h"
#include "libuarm.h"
#include "interrupts.h"
#include "timers.h"

// extern uint64_t millisecondsSinceStart;

void LDST(void *state) {
    // TODO: prevent loading EL1 from EL0
    int el = SYSCALL(SYS_GETCURRENTEL, 0, 0, 0);
    if (el == 0) {
        SYSCALL(SYS_LAUNCHSTATE, (long unsigned int)state, 0, 0);
    } else {
        LDST_EL0(state);
    }
}

void STST(void *state) {
    int el = SYSCALL(SYS_GETCURRENTEL, 0, 0, 0);
    STST_EL0(state);
    ((state_t *)state)->status_register = SYSCALL(SYS_GETCURRENTSTATUS, 0, 0, 0);
    if (el == 0) {
        ((state_t *)state)->TTBR0 = SYSCALL(SYS_GETTTBR0, 0, 0, 0);
    }
}