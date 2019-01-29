#include "listx.h"
#include "utils.h"
#include "emulated_timers.h"
#include "uart.h"

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


struct list_head pendingTimers = LIST_HEAD_INIT(pendingTimers);
struct list_head freeTimers    = LIST_HEAD_INIT(freeTimers);


// inizializza la lista pcbFree
void init_emulated_timers() {
    static timer_t timer_array[MAX_TIMERS];
    int            i;
    for (i = 0; i < MAX_TIMERS; i++) {
        list_add_tail(&timer_array[i].list, &freeTimers);
    }
}

// Alloca un pcb dalla lista pcbFree
timer_t *allocTimer() {
    if (list_empty(&freeTimers)) {
        LOG(ERROR, "Fatal: unable to allocate timer");
        return NULL;
    }
    else {
        timer_t *block;
        block = container_of(freeTimers.next, timer_t, list);
        list_del(freeTimers.next);
        memset(block, 0, sizeof(block)); /*setta tutti i campi di pcb a 0 (NULL) */
        return (block);
    }
}


// Aggiungi p alla lista di pcb liberi
void freeTimer(timer_t *p) { list_add_tail(&p->list, &freeTimers); }


/*PROCESS QUEUE MAINTENANCE*/

// Inserisce p nella lista la cui list_head è q
void insertTimer(struct list_head *q, timer_t *p) {
    if (q == NULL || p == NULL)
        return;

    struct list_head *aux, *old;
    timer_t *         tmp;

    q = (struct list_head*)((uint64_t)q & 0x0000ffffffffffff);

    old = q;
    aux = q->next;
    while (aux != q && aux != NULL) {
        tmp = container_of(aux, timer_t, list);
        if (tmp->time > p->time)
            break;

        old = aux;
        aux = aux->next;
    }

    if (aux == q)
        list_add_tail(&(p->list), q);
    else
        list_add(&(p->list), old);
}


// Rimuove il primo pcb dalla lista la cui list_head è puntata da q
timer_t *removeTimer(struct list_head *q) {
    q = (struct list_head*)((uint64_t)q & 0x0000ffffffffffff);
    if (q == NULL || list_empty(q))
        return NULL;
    timer_t *first;
    first = container_of(q->next, typeof(*first), list);
    list_del(q->next);
    return first;
}

void removeTimerType(struct list_head *q, TIMER_TYPE type, int code) {
    struct list_head *aux, *del;
    timer_t *         tmp;

    q = (struct list_head*)((uint64_t)q & 0x0000ffffffffffff);

    if (q == NULL)
        return;

    aux = q->next;
    while (aux != q && aux != NULL) {
        tmp = container_of(aux, timer_t, list);
        if (tmp->type == type && tmp->code == code) {
            del = aux;
            aux = aux->next;
            list_del(del);
            freeTimer(container_of(del, timer_t, list));
        } else {
            aux = aux->next;
        }
    }
}


// Restituisce il primo pcb dalla lista la cui list_head è puntata da q, senza rimuoverlo
timer_t *headTimer(struct list_head *q) {
    q = (struct list_head*)((uint64_t)q & 0x0000ffffffffffff);
    if (q == NULL || list_empty(q))
        return NULL;
    q = q->next;
    return container_of(q, timer_t, list);
}


void add_timer(uint64_t time, TIMER_TYPE type, int code) {
    timer_t *timer;
    
    removeTimerType(&pendingTimers, type, code);
    timer = allocTimer();

    if (timer == NULL) {
        LOG(ERROR, "Fatal: no more timers!");
        return;
    }

    timer->time = time;
    timer->type = type;
    timer->code = code;

    insertTimer(&pendingTimers, timer);
}

int next_timer(timer_t *next) {
    timer_t *timer = headTimer(&pendingTimers);
    if (timer == NULL)
        return -1;

    memcpy(next, timer, sizeof(timer_t));
    return 0; 
}

int next_pending_timer(uint64_t currentTime, timer_t *next) {
    timer_t *timer = headTimer(&pendingTimers);
    if (timer == NULL)
        return -1;

    memcpy(next, timer, sizeof(timer_t));
    if (timer->time < currentTime) {
        removeTimer(&pendingTimers);
        freeTimer(timer);
        return 1;
    }
    return 0;
}