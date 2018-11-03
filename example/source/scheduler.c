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

//Modulo che implementa lo scheduler

#include "libuarm.h"
#include "arch.h"
#include "uARMconst.h"
#include "uARMtypes.h"
#include "const.h"

#include "asl.h"   

#include "initial.h"
#include "scheduler.h"


//ultima partenza del time slice
unsigned int last_slice_start = 0;
//per contare lo userTime del processo corrente
unsigned int userTimeStart = 0;
//per contare il CPUTime del processo corrente
unsigned int CPUTimeStart = 0;
//ultima partenza dello pseudo clock
unsigned int pseudo_clock_start = 0;
//quale dei due timer ( pseudo clock o time slice ) alzerà un interrupt per primo
unsigned int current_timer;


//funzione che setta il prossimo timer, che è quello più vicino tra lo pseudo clock e il time slice del processo corrente
void timer(){
	unsigned int time = getTODLO();
	int slice_end = SCHED_TIME_SLICE - (time - last_slice_start );//tempo che manca alla fine del time slice corrente
	int clock_end = SCHED_PSEUDO_CLOCK - (time - pseudo_clock_start);//tempo che manca alla fine dello pseudo clock tick corrente
	if( slice_end <= 0){ //time slice terminato, setta il prossimo
		last_slice_start = time;
		slice_end = SCHED_TIME_SLICE;
	}
	if( clock_end <=0 ){	//pseudo clock terminato, setta il prossimo
		pseudo_clock_start = time;
		clock_end = SCHED_PSEUDO_CLOCK;
	}
	//settiamo il prossimo timer, che deve essere il clock o il time slice, a seconda di quale occorrerà prima
	if( clock_end <= slice_end ){
		setTIMER(clock_end);
		current_timer = PSEUDO_CLOCK;
	}
	else{
		setTIMER(slice_end);
		current_timer = TIME_SLICE;
	}
}


void scheduler(){
	timer();
	//sul processo idle faccio preemption, per evitare di aspettare che il suo time slice sia finito anche se
	//ci sarebbero altri processi pronti
	if( (current_process != NULL) && (current_process->priority == PRIO_IDLE)){
		insertProcQ( priority_queue( current_process->priority), current_process );
		current_process = NULL;
	}
	if( current_process == NULL){
		//devo attivare un processo in attesa
		if( !list_empty(priority_queue(PRIO_HIGH))) current_process = removeProcQ(priority_queue(PRIO_HIGH));
		else if( !list_empty(priority_queue(PRIO_NORM))) current_process = removeProcQ(priority_queue(PRIO_NORM));
		else if( !list_empty(priority_queue(PRIO_LOW))) current_process = removeProcQ(priority_queue(PRIO_LOW));
		else{
			if( process_count == 0 ){	//non ci sono più processi e posso terminare
				HALT();
			}
			else if( (process_count > 0) && (softblock_count == 0 )){	//deadlock
				PANIC();	
			}
			else{
				current_process = removeProcQ(priority_queue(PRIO_IDLE));	//i processi attivi sono tutti in attesa,
				if( current_process == NULL ){  PANIC();}			//attivo il processo idle
			}
		}
		CPUTimeStart = getTODLO();	//se comincia l'esecuzione di un nuovo processo riparte il conteggio del tempo di CPU
	}
	userTimeStart = getTODLO();	//riparte il conteggio del tempo utente
	LDST( &current_process->p_s );
}
