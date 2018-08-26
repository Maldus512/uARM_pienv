#ifndef __ASMLIB_H__
#define __ASMLIB_H__

#include "hardwareprofile.h"

void hang(unsigned int d);
uint32_t GETEL();
uint32_t GETSAVEDSTATE();
uint32_t GETARMCLKFRQ();
uint32_t GETARMCOUNTER();


#endif