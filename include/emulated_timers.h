#ifndef __EMULATED_TIMERS_H__
#define __EMULATED_TIMERS_H__

#include "arch.h"
#include "listx.h"

typedef enum {
    UNALLOCATED = 0,
    TIMER,
    PRINTER,
    TAPE,
    DISK,
} TIMER_TYPE;

typedef struct {
    uint64_t         time;
    TIMER_TYPE       type;
    int              code;
    struct list_head list;
} timer_t;

#define     MAX_TIMERS      ((IL_LINES+1)*MAX_DEVICES)

void init_emulated_timers();
void add_timer(uint64_t time, TIMER_TYPE type, int code);
int next_pending_timer(uint64_t currentTime, timer_t *next);
int next_timer(timer_t *next);

#endif