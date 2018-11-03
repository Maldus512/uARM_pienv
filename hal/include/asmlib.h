#ifndef __ASMLIB_H__
#define __ASMLIB_H__

#include "hardwareprofile.h"

void hang(unsigned int d);
uint32_t GETEL();
uint32_t GETSAVEDSTATE();
uint32_t GETARMCLKFRQ();
uint32_t GETARMCOUNTER();

void DOWFI();

void enable_irq();
void disable_irq();

uint64_t STELR();
uint64_t GETSP_EL0();
void LDELR(uint64_t);
uint32_t GETSAVEDEL();
void LDST_EL0(void *state);
#endif