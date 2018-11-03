/*  Copyright (C) 2015  Carlo Stomeo, Stefano Mazza, Alessandro Zini, Mattia Maldini

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

//Modulo di inizializzazione

#include "libuarm.h"
#include "arch.h"
#include "uARMconst.h"
#include "uARMtypes.h"
#include "const.h"

#include "asl.h"
#include "pcb.h"


//macro per disabilitare la memoria virtuale
#define ENABLE_VM 0x00000001


extern void test();

//variabili di sitema: contatore dei processi attivi e dei processi bloccati su un device
unsigned int process_count;
unsigned int softblock_count;

//code di processi ready con varie priorità
LIST_HEAD(rdyQueue_low);
LIST_HEAD(rdyQueue_norm);
LIST_HEAD(rdyQueue_high);
LIST_HEAD(rdyQueue_idle);

//provesso corrente
struct pcb_t *current_process;

//semafori dei device
int devSem[MAX_DEVICES];

//device register status da salvare per essere restituito dalla SYS8
unsigned int devStatus[MAX_DEVICES];
//tempo usato per gestire l'ultimo interrupt su ogni device
cputime_t interruptTime[MAX_DEVICES];

struct pcb_t *first, *twiddle;	//primo processo da attivare e processo idle

int pid_map[MAXPROC];	                //bitmap per i pid disponibili
struct pcb_t* activeProcesses[MAXPROC];	//insieme dei processi attivi, indicizzati per pid

//funzione che restituisce l'indice del semaforo del device corrispondente nell'array devSem
int devSemIndex( int int_line, int dev_num ){
	int_line -= 3;	//le linee di interrupt vanno da 0 a 7 ma i device con semafori sono da 3 a 7
	if( (int_line*DEV_PER_INT + dev_num < 0) || (int_line*DEV_PER_INT + dev_num > 48) ) PANIC();
	return int_line*DEV_PER_INT + dev_num;
}

//funzione che inizializza un'area con il gestore passato come parametro
void init_area(memaddr area, memaddr handler){	
//i parametri sono l'area da inizializzare e il gestore relativo
	state_t *newArea = (state_t*) area;
	STST(newArea);
	newArea->pc = handler;
	newArea->sp = RAM_TOP;
	//interrupt disabilitati e kernel-mode
	newArea->cpsr = STATUS_ALL_INT_DISABLE((newArea->cpsr) | STATUS_SYS_MODE);
}

//funzione che data la priorità restituisce il puntatore alla lista corrispondente
struct list_head* priority_queue( int priority ){	
	switch(priority){
		case PRIO_LOW:
			return &rdyQueue_low;
			break;
		case PRIO_NORM:
			return &rdyQueue_norm;
			break;
		case PRIO_HIGH:
			return &rdyQueue_high;
			break;
		case PRIO_IDLE:
			return &rdyQueue_idle;
			break;
	}
}

//copia lo stato da from a to
void copy_state(state_t *from, state_t *to){	
	to->a1 = from->a1;
	to->a2 = from->a2;
	to->a3 = from->a3;
	to->a4 = from->a4;
	to->v1 = from->v1;
	to->v2 = from->v2;
	to->v3 = from->v3;
	to->v4 = from->v4;
	to->v5 = from->v5;
	to->v6 = from->v6;
	to->sl = from->sl;
	to->fp = from->fp;
	to->ip = from->ip;
	to->sp = from->sp;
	to->lr = from->lr;
	to->pc = from->pc;
	to->cpsr = from->cpsr;
	to->CP15_Control = from->CP15_Control;
	to->CP15_EntryHi = from->CP15_EntryHi;
	to->CP15_Cause = from->CP15_Cause;
	to->TOD_Hi = from->TOD_Hi;
	to->TOD_Low = from->TOD_Low;
}

//dato il puntatore alla linea di interrupt (ottenuto con la macro CDEV_BITMAP_ADDR)
//ottengo il device di quel tipo su cui pende un interrupt
int pending_interrupt_device( memaddr* line ){	
	int i;
	int dev_num = *line;
	for( i = 0; i < 8; i++){
		if(dev_num & (1 << i)){
			dev_num = i;
			break;
		}
	}
	return dev_num;
}

//Processo idle, in attesa perenne. Che vita triste. O no?
void idle(){	
	while(1) WAIT();
}

void processSet( struct pcb_t *p, memaddr start, int priority){
	// Abilita gli interrupt, il Local Timer e la kernel-mode
  	p->p_s.cpsr = STATUS_ALL_INT_ENABLE(p->p_s.cpsr) | STATUS_SYS_MODE;
    	// Disabilita la memoria virtuale
    	p->p_s.CP15_Control = (p->p_s.CP15_Control) & ~(ENABLE_VM);
	p->p_s.cpsr = STATUS_ENABLE_TIMER(p->p_s.cpsr);
    	p->p_s.sp = RAM_TOP - FRAME_SIZE;
    	// Assegna a PC l'indirizzo della funzione esterna test()
    	p->p_s.pc = (memaddr) start;
	p->priority = priority;
    	//assegno un pid al processo
	//p->pid = newpid();
	activeProcesses[first->pid-1] = p;
	//inserisco il processo dove di competenza
	insertProcQ(priority_queue( priority ), p);
}

int p2test_init(){
	int i, j;
	state_t *statep;
	//popolo le aree della ROM
	init_area(INT_NEWAREA, (memaddr) test);
	init_area(TLB_NEWAREA, (memaddr) test);
	init_area(PGMTRAP_NEWAREA, (memaddr) test);
	init_area(SYSBK_NEWAREA, (memaddr) test);
	//inizializzo le strutture di phase1
	initPcbs();
	initASL();
	//inizializzo le variabili di sistema
	process_count = 0;
	softblock_count = 0;
	//mappa dei process id liberi
	for( i = 0; i < MAXPROC ; i++ ) pid_map[i] = 0;
	//array di puntatori ai processi attivi
	for( i = 0; i < MAXPROC ; i++ ) activeProcesses[i] = NULL;
	//semafori dei device
	for(i = 0; i < MAX_DEVICES; i++) devSem[i] = 0;
    	//registri di stato da conservare dei device
	for(i = 0; i < MAX_DEVICES; i++) devStatus[i] = 0;
	//tempo di gestione degli interrupt
	for(i = 0; i < MAX_DEVICES; i++) interruptTime[i] = 0;

	if( !((first = allocPcb()) && (twiddle = allocPcb()))){
		PANIC();
	}

	processSet( twiddle, (memaddr) idle, PRIO_IDLE);	//il processo idle ha id 1
	
	processSet( first, (memaddr) test, PRIO_NORM );

	current_process = NULL;
	process_count++;
    tprint("fase 2 inizializzata con successo\n");
	scheduler();
}

