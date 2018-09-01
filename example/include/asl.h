#ifndef _ASL_H
#define _ASL_H
 
  
#include "listx.h"
#include "pcb.h"
 
 
/* semaphore descriptor */
struct semd_t{
    int *s_semAdd ; /* pointer to the semaphore */
    struct list_head s_link ; /* ASL linked list */
    struct list_head s_procq ; /* blocked process queue */
    int waitingFor; //numero del tipo di risorsa che si sta aspettando
};
 
 
/* process control block, defined in pcb.h*/
struct pcb_t ;
 

/*ACTIVE SEMAPHORE LIST*/
HIDDEN inline int* address(struct list_head* p);
 

void initASL();
 
 
int insertBlocked(int *semAdd, struct pcb_t *p);
 
  
struct pcb_t *removeBlocked(int *semAdd);
  
  
struct pcb_t *outBlocked(struct pcb_t *p);
  

struct pcb_t *headBlocked(int *semAdd);
  
#endif
