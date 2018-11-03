#ifndef _INITIAL_H
#define _INITIAL_H

unsigned int process_count;
unsigned int softblock_count;
/*
LIST_HEAD(rdyQueue_low);
LIST_HEAD(rdyQueue_norm);
LIST_HEAD(rdyQueue_high);
LIST_HEAD(rdyQueue_idle);*/

struct list_head rdyQueue_norm;
struct pcb_t *current_process;

void iprint(char *msg);

unsigned int status_iprint;

int devSem[MAX_DEVICES];

cputime_t interruptTime[MAX_DEVICES];

unsigned int devStatus[MAX_DEVICES];

struct pcb_t *first, *twiddle;

int pid_map[MAXPROC];

struct pcb_t* activeProcesses[MAXPROC];

void init_area(memaddr area, memaddr handler);

struct list_head* priority_queue( int priority );

int devSemIndex( int int_line, int dev_num );

int pending_interrupt_device( memaddr* line );

#endif
