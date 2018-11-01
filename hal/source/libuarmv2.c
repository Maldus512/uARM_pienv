#include "asmlib.h"
#include "libuarmv2.h"
#include "uARMtypes.h"

typedef struct _state64_t {
    uint64_t xreg[28];
    uint64_t fp;
    uint64_t lr;
    uint64_t elr;
} state64_t;



void STST_c(void *addr) {
    state_t *state = addr;
    state->pc = LDELR();
}