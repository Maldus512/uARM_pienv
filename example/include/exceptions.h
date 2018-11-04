#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H

void tlb_handler();

void trap_handler();

void specified_handler(int ex_type);

typedef enum {
	//SYSBK = 0,//1 3 5
	//PGMTRAP = 1,
	//TLB = 2,
	TLB = 0,
	SYSBK = 1,
	PGMTRAP = 2,
} specExEnum;


#endif
