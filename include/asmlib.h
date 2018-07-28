#ifndef __ASMLIB_H__
#define __ASMLIB_H__

uint32_t GETSP();
uint32_t GETCPSR();
uint32_t GET32(uint32_t add);
void PUT32(uint32_t add, uint32_t val);

#endif