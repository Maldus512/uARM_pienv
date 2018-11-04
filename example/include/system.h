#ifndef _SYSTEM_H
#define _SYSTEM_H

void sys_handler();

void createProcess(state_t *state, int priority);

void terminateProcess(pid_t pid);

void verhogen(int* semaddr, int weight, unsigned int p_a1reg, cputime_t time);

void passeren( int* semaddr, int weight );

void specTrapVec( state_t **state_vector );

void getCpuTime( cputime_t *global, cputime_t *user );

void waitClock();

void waitIO( int intlNo, int dnum, int waitForTermRead );

void getPid();

void getPPid();

#endif
