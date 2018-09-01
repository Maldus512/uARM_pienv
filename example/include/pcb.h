#ifndef _PCB_H
#define _PCB_H

#include "const.h"
#include "listx.h"
//#include <unistd.h>

struct areas {
    state_t *OldArea;
    state_t *newArea;
};
 
/* process control block type */
struct pcb_t {
    cputime_t CPUTime, userTime;
    state_t *TrapVec[6];	// Old-New areas: 0-1 sysbreakpoints, 2-3 trap, 4-5 tlb
    int priority;
    pid_t pid;
    struct list_head p_list; /* process list */
    struct list_head p_children; /* children list entry point*/
    struct list_head p_siblings; /* children list: links to the siblings */
    struct pcb_t *p_parent; /* pointer to parent */
    struct semd_t *p_cursem; /* pointer to the semd_t on which process blocked */
    state_t p_s;
};
 
/*ALLOCATION AND DEALLOCATION OF PROCBLOCKS*/
HIDDEN void *memset_(void *dst, int c, size_t n);
 
 
extern void initPcbs(void);

 
struct pcb_t *allocPcb();


void freePcb(struct pcb_t *p);


/*PROCESS QUEUE MAINTENANCE*/
void insertProcQ(struct list_head *q, struct pcb_t *p);
 

struct pcb_t *removeProcQ(struct list_head *q);
 

struct pcb_t *outProcQ(struct list_head *q, struct pcb_t *p);
     
  
struct pcb_t *headProcQ(struct list_head *q);


/*PROCESS TREE MAINTENANCE*/
int emptyChild(struct pcb_t *p);


int insertChild(struct pcb_t *parent, struct pcb_t *p);

 
struct pcb_t *outChild(struct pcb_t *p);

 
struct pcb_t *removeChild(struct pcb_t *p);



#endif
