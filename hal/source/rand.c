#include "rand.h"
/**
 * Initialize the RNG
 */
void initRand()
{
    *RNG_STATUS=0x40000;
    // mask interrupt
    *RNG_INT_MASK|=1;
    // enable
    *RNG_CTRL|=1;
    // wait for gaining some entropy
    while(!((*RNG_STATUS)>>24)) asm volatile("nop");
}

/**
 * Return a random number between [min..max]
 */
unsigned int rand(unsigned int min, unsigned int max)
{
    return *RNG_DATA % (max-min) + min;
}