#include "uARMconst.h"
#include "uARMtypes.h"
#include "libuarm.h"
#include "listx.h"
#include "pcb.h"

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

//Modulo che implementa le funzioni di gestione dei process control block

//Lista di pcb liberi 
HIDDEN LIST_HEAD(pcbFree); 


/*ALLOCATION AND DEALLOCATION OF PROCBLOCKS*/


HIDDEN void *memset_(void *dst, int c, size_t n){	//presa dalla libreria string.h per settare tutti i campi
     if (n){                  				//delle strutture pcb_t a 0
         char *d = dst;
         do{
             *d++ = c;
         }while(--n);
     }
     return dst;
 }


//inizializza la lista pcbFree
void initPcbs(void){
    HIDDEN struct pcb_t pcb_array[MAXPROC];
    int i;
    for(i=0; i<20; i++){
        list_add_tail(&pcb_array[i].p_list, &pcbFree);
    }
}

//Alloca un pcb dalla lista pcbFree
struct pcb_t *allocPcb(){
    if (list_empty( &pcbFree))
        return NULL;
    else{
       struct pcb_t *block;
       block = container_of(pcbFree.next,struct pcb_t, p_list); 
       list_del(pcbFree.next);                   
       memset_( block , 0 , sizeof( block ) ) ;  /*setta tutti i campi di pcb a 0 (NULL) */
       return(block);
    }
}


//Aggiungi p alla lista di pcb liberi
void freePcb(struct pcb_t *p){
    list_add_tail( &p->p_list, &pcbFree);
}


/*PROCESS QUEUE MAINTENANCE*/

//Inserisce p nella lista la cui list_head è q
void insertProcQ(struct list_head *q, struct pcb_t *p){ 
    if ( !(q == NULL || p == NULL) ) list_add_tail( &(p->p_list) , q );
}
 

//Rimuove il primo pcb dalla lista la cui list_head è puntata da q
struct pcb_t *removeProcQ(struct list_head *q){
     if (q == NULL || list_empty(q)) return NULL; 
     struct pcb_t *first; 
     first = container_of( q->next, typeof(*first), p_list );
     list_del(q->next);
     return first;
}
 

//Rimuove p dalla lista la cui list_head è puntata da q
struct pcb_t *outProcQ(struct list_head *q, struct pcb_t *p){
    if (p == NULL || q == NULL) return NULL;
    struct pcb_t *tmp;
    struct list_head *aux = q->next;
    while ( aux != q && container_of( aux, typeof(*tmp), p_list ) != p ) aux = aux->next ;
    tmp = container_of( aux, typeof(*tmp), p_list ) ;
    if( tmp == p ){
        list_del( &(tmp->p_list) );
        return tmp ;
    }
    else return NULL ;
}
  
   
//Restituisce il primo pcb dalla lista la cui list_head è puntata da q, senza rimuoverlo
struct pcb_t *headProcQ(struct list_head *q){
    if (q == NULL || list_empty(q) ) return NULL; /*same as removeProcQ*/
    q = q->next ;
    return container_of( q, struct pcb_t , p_list );
}


/*PROCESS TREE MAINTENANCE*/

//true se p non ha figli, false se ne ha almeno uno
int emptyChild(struct pcb_t *p) {
    if( p->p_children.next == NULL ) return 1; // un list_head i cui campi puntano a NULL si intende come una lista vuota
    else return list_empty( &(p->p_children) );
}
 

//Rende p un figlio di parent, inserendolo nella sua lista di processi figli
int insertChild(struct pcb_t *parent, struct pcb_t *p) {
    if (parent == NULL || p == NULL) return 1;
    if( parent->p_children.next == NULL ) {	//Se i puntatori sono ancora a NULL li inizializziamo correttamente
        INIT_LIST_HEAD( &parent->p_children );
    }
    list_add_tail( &(p->p_siblings), &(parent->p_children) ); 
    p->p_parent = parent;
    return 0;
}


//Rimuove p dalla lista dei figli di suo padre
struct pcb_t *outChild(struct pcb_t *p) {
    if ( p == NULL || p->p_parent == NULL ) return NULL; 
    list_del( &(p->p_siblings) );   
    p->p_parent = NULL;
    return p;
}

 
//Rimuove il primo processo figlio di p
struct pcb_t *removeChild(struct pcb_t *p) {
    if ( p == NULL || emptyChild(p) ) return NULL;
    struct pcb_t *removedChild;
    struct list_head *q = &p->p_children;
    if (q == NULL || list_empty(q)) return NULL; //return NULL se q è NULL o la lista dei figli di p è vuota
    removedChild = container_of( q->next, typeof(*removedChild), p_siblings );
    list_del(q->next);
    removedChild->p_parent = NULL;
    return removedChild;
}



