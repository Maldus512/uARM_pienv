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


#include "uARMconst.h"
#include "uARMtypes.h"
#include "libuarm.h"
#include "listx.h"

#include "pcb.h"
#include "asl.h"



/*Modulo contenente le funzione di gestione delle liste di semafori*/


//funzione ausiliaria; dato un list_head dentro un semd_t, restituisce il campo s_semAdd
HIDDEN inline int* address(struct list_head* p){
    return ( ( container_of( p , struct semd_t , s_link ) )->s_semAdd );
}

//lista di semafori liberi
HIDDEN LIST_HEAD(semdFree);
//lista di semafori attivi
HIDDEN LIST_HEAD(aslh);

//rimuove il semaforo q dalla lista di quelli attivi se non ha processi bloccati
HIDDEN inline void rem_idle_sem( struct semd_t* q){
	if( list_empty( &(q->s_procq) )){
		list_del( &(q->s_link) );
		list_add( &(q->s_link), &semdFree );
	}
}


//descrittori di semafori disponibili
HIDDEN struct semd_t semdTable[MAXPROC];
 

//funzione ausiliaria che, dato un indirizzo di semaforo, restituisce il descrittore di semaforo attivo
// per esso se esiste sulla ASL, NULL in caso contrario
struct semd_t* semd_by_add(int* semAdd){
	struct list_head *aux ;
	aux = aslh.next ;
	while( aux != &aslh  && (address(aux) < semAdd) ) aux = aux->next;	/*browsing the ASL until the s_semAdd field in the semd_t containing aux is >= semAdd ( the ASL list is sorted in ascending order) or until it's finished; either way, the semd_t we're looking for will be the one containing  the list_head pointed by aux if already present; if not, the new one will be placed between aux->prev and aux*/
	if( address(aux) == semAdd ) return container_of( aux , struct semd_t, s_link ) ;
	else return NULL ;
}


//inizializza la lista di semafori liberi
void initASL(){
    int i;
    for( i = 0; i < MAXPROC; i++ ){
    INIT_LIST_HEAD( &semdTable[i].s_procq ) ;
        list_add( &(semdTable[i].s_link) , &(semdFree) );
    }
}
 

//inserisce il processo p nella lista dei processi bloccati del semaforo puntato da semAdd
int insertBlocked(int *semAdd, struct pcb_t *p){
    if( p == NULL ) return 2;   //condizione di errore
    struct list_head *aux , *tmp;  
    struct semd_t* q;
    aux = aslh.next;
    q = semd_by_add( semAdd ) ;
    if( q == NULL ){   //caso in cui il semaforo non è già presente e dobbiamo aggiungerlo
        if( list_empty( &(semdFree) )) return 1;    //non ci sono semafori disponibili
	while( aux != &aslh  && (address(aux) < semAdd) ) aux = aux->next; //cerchiamo il punto dove inserire il nuovo semaforo
        tmp = semdFree.next;
        list_del(tmp);
        list_add(tmp , aux->prev ); //manteniamo la lista ordinata per indirizzo dei semafori
        q = container_of( tmp , struct semd_t , s_link );
        q->s_semAdd = semAdd;
    }
    p->p_cursem = q;
    list_add_tail(&(p->p_list) , &(q->s_procq));
    return 0;
}


//Rimuove il primo processo dalla lista dei processi bloccati del semaforo puntato da semAdd  
struct pcb_t *removeBlocked(int *semAdd){
    struct semd_t* q;
    struct pcb_t* p ;
    q = semd_by_add( semAdd );
    if( q != NULL ){
	p = removeProcQ( &(q->s_procq) );
        rem_idle_sem(q);
	return p ;	
    }
    else return NULL;   /*return NULL se nessun semAdd non viene trovato*/
}


//Rimuove il processo p dalla lista di processi bloccati del semaforo su cui è bloccato
//(se bloccato su un semaforo)
struct pcb_t *outBlocked(struct pcb_t *p){
    if( p == NULL ) return NULL;
    struct semd_t *sem ;
    if( p->p_cursem == NULL ) return NULL;
    sem = semd_by_add( p->p_cursem->s_semAdd );
    if( sem == NULL ) return NULL ;
    p = outProcQ(  &(sem->s_procq) , p ); 
    if(p!= NULL) rem_idle_sem( p->p_cursem );
    return p;
}
 

//Restituisce il primo processo bloccato sul semaforo puntato da semAdd senza rimuoverlo
struct pcb_t *headBlocked(int *semAdd){
    struct semd_t* q;
    q = semd_by_add( semAdd );
    if( q == NULL ) return NULL;
    return headProcQ( &q->s_procq );
}
