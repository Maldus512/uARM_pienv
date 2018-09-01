#ifndef CONST_H
#define CONST_H

#include "arch.h"

#define MAXPROC 20
#define NULL ((void *)0)
#define TRUE 1
#define FALSE 0

#define N_DEV_TYPES (N_EXT_IL + 1)

#define HIDDEN static

typedef unsigned int U32;
typedef signed int S32;
typedef unsigned char U8;
typedef signed char S8;

typedef unsigned int memaddr;
typedef int pid_t;
typedef unsigned int cputime_t;

typedef enum {
	PRIO_IDLE = 0,
	PRIO_LOW = 1,
	PRIO_NORM = 2,
	PRIO_HIGH = 3
} priority_enum;

typedef enum {
	EXCP_TLB_OLD,
	EXCP_TLB_NEW,
	EXCP_SYS_OLD,
	EXCP_SYS_NEW,
	EXCP_PGMT_OLD,
	EXCP_PGMT_NEW
} exceptionAreas_enum;

#endif
