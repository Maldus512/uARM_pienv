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

//Modulo che contiene il gestore degli interrupt


#include "libuarm.h"
#include "arch.h"
#include "uARMconst.h"
#include "uARMtypes.h"
#include "const.h"

#include "asl.h"
#include "pcb.h"

#include "initial.h"
#include "scheduler.h"
#include "interrupt.h"
#include "system.h"
#include "exceptions.h"

cputime_t interruptStart;	//tempo di inizio della gestione dell'interrupt

//disk, tape, network, printer interrupt handler
void dtnp_interrupt(int int_line){	
	dtpreg_t *dev_g;
	int i;
	unsigned int status;		//valore da far ritornare alla SYS8
	memaddr *interrupt_line = (memaddr*) CDEV_BITMAP_ADDR(int_line);	//ottengo la linea di interrupt
	int dev_num = pending_interrupt_device(interrupt_line);	//ottengo il device su cui pende l'interrupt
	dev_g = (dtpreg_t*) DEV_REG_ADDR(int_line, dev_num);
	dev_g->command = DEV_C_ACK;	//passo l'acknowledgement
	i = devSemIndex(int_line, dev_num) ;
	if( devSem[i] < 1 ){				//se ci sono dei processi bloccati sul semaforo
		status = dev_g->status;			//li libero e passo loro lo status register e il tempo di gestione
		//verhogen( &devSem[i] , 1 , status,  getTODLO() - interruptStart);
		devStatus[i] = 0;
	}
	else{	//se nessuno sta aspettando l'interrupt salvo lo status register e il tempo di gestione dell'interrupt
		// per quando qualcuno li richiederà
		devStatus[i] = dev_g->status;
		interruptTime[i] = getTODLO() - interruptStart;
	}
}

//gestore degli interrupt per i terminali
void terminal_interrupt(){	
	termreg_t *dev_t;
	int i ;
	memaddr *interrupt_line = (memaddr*) CDEV_BITMAP_ADDR(IL_TERMINAL);	//ottengo la linea di interrupt per i terminali
	int dev_num = pending_interrupt_device(interrupt_line);	//ottengo il terminale su cui pende l'interrupt
	dev_t = (termreg_t*) DEV_REG_ADDR(IL_TERMINAL, dev_num);	//ottengo il registro del terminale
	unsigned int status ;      //valore da far ritornare alla SYS8
	status_iprint = dev_t->transm_status;
	if( (dev_t->transm_status & DEV_TERM_STATUS ) == DEV_TTRS_S_CHARTRSM){
		//trattasi di una scrittura, priorità più alta della lettura
		status = dev_t->transm_status;
		i = devSemIndex(IL_TERMINAL+1, dev_num);
		dev_t->transm_command = DEV_C_ACK;	//acknowledgement
		if( devSem[i] < 1 ){				//se ci sono dei processi bloccati sul semaforo
			//verhogen(&devSem[i], 1, status, getTODLO() - interruptStart);	//li libero e passo loro lo status register
			devStatus[i] = 0;
		}
		else{	//se nessuno sta aspettando l'interrupt salvo lo status register per quando qualcuno lo richiederà
			devStatus[i] = dev_t->transm_status;
			interruptTime[i] = getTODLO() - interruptStart;	//tempo impegato a gestire l'interrupt
		}
	}
	else if( (dev_t->recv_status & DEV_TERM_STATUS) == DEV_TRCV_S_CHARRECV ){
		//trattasi di una lettura
		status = dev_t->recv_status;
		i = devSemIndex(IL_TERMINAL, dev_num);
		dev_t->recv_command = DEV_C_ACK;	//acknowledgement
		if( devSem[i] < 1 ){
			//verhogen(&devSem[i], 1,status, getTODLO() - interruptStart);	//come sopra
			devStatus[i] = 0;
		}
		else{
			devStatus[i] = dev_t->recv_status;
			interruptTime[i] = getTODLO() - interruptStart;	//tempo impegato a gestire l'interrupt
		}
	}
	
}

//gestore degli interrupt
void int_handler(){	
	interruptStart = getTODLO();	//tempo in cui comincia la gestione dell'interrupt, da
					//assegnare poi ad un eventuale processo svegliato dall'interrupt
	state_t *returnState = (state_t*) INT_OLDAREA;
	returnState->pc -= 4;
	if( current_process != NULL){
		copy_state( returnState, &current_process->p_s );
		current_process->userTime += interruptStart - userTimeStart;//se c'è un processo aggiorno il suo userTime
		current_process->CPUTime += interruptStart - CPUTimeStart;
	}
	int cause = getCAUSE();
	if(CAUSE_IP_GET(cause, IL_TIMER)){
		if( current_timer == PSEUDO_CLOCK ){
			while( devSem[CLOCK_SEM] < 0 ){	//Ad ogni pseudo clock tick mi assicuro che il semaforo sia sempre a zero
				//verhogen( &devSem[CLOCK_SEM], 1, 0, getTODLO() - interruptStart);
			}
		}
		else if(current_timer == TIME_SLICE ){
			if( current_process != NULL ){
				insertProcQ( priority_queue(current_process->priority), current_process);
				current_process->CPUTime += getTODLO() - interruptStart;	//se metto in pausa il processo aggiorno anche il suo CPUTime
				current_process = NULL;
			}
		}
	}
	else if(CAUSE_IP_GET(cause, IL_DISK)){
		dtnp_interrupt(IL_DISK);
	}
	else if(CAUSE_IP_GET(cause, IL_TAPE)){
		dtnp_interrupt(IL_TAPE);
	}
	else if(CAUSE_IP_GET(cause, IL_ETHERNET)){
		dtnp_interrupt(IL_ETHERNET);
	}
	else if(CAUSE_IP_GET(cause, IL_PRINTER)){
		dtnp_interrupt(IL_PRINTER);
	}
	else if(CAUSE_IP_GET(cause, IL_TERMINAL)){
		terminal_interrupt();
	}
	scheduler();
}
