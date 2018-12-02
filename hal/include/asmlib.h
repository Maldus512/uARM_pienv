#ifndef __ASMLIB_H__
#define __ASMLIB_H__

#include "hardwareprofile.h"

uint32_t GETEL();
uint32_t GETSAVEDSTATUS();
uint32_t GETARMCLKFRQ();
uint32_t GETARMCOUNTER();

void enable_irq();
void disable_irq();

uint64_t GETTTBR0();
uint64_t STELR();
uint64_t GETSP_EL0();
void     LDELR(uint64_t);
uint32_t GETSAVEDEL();
void     LDST_EL0(void *state);
void     STST_EL0(void *state);
#endif