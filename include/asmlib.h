#ifndef __ASMLIB_H__
#define __ASMLIB_H__

#include "arch.h"

uint32_t GETEL();
uint32_t GETARMCLKFRQ();
uint32_t ISMMUACTIVE();
void     RIGVBAR();

uint64_t GETTTBR0();
uint64_t GETTTBR1();
uint64_t STELR();
void     LDELR(uint64_t);
uint64_t GETSP();
void SETSP(uint64_t sp);
uint32_t GETSAVEDEL();
void     CoreExecute(unsigned int core, void *task);

void restoreEL2SP();
#endif