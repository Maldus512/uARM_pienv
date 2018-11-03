#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#define PSEUDO_CLOCK 0
#define TIME_SLICE 1

unsigned int last_slice_start;
unsigned int userTimeStart;
unsigned int CPUTimeStart;
unsigned int pseudo_clock_start;
unsigned int current_timer;

void timer();

void scheduler();

#endif
