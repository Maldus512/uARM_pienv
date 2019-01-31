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

#include "listx.h"
#include "utils.h"
#include "emulated_timers.h"
#include "uart.h"


struct list_head pending_timers = LIST_HEAD_INIT(pending_timers);
struct list_head free_timers    = LIST_HEAD_INIT(free_timers);


/* 
 * Initializes the free_timers list
 */
void init_emulated_timers() {
    static timer_t timerarray[MAX_TIMERS];
    int            i;
    for (i = 0; i < MAX_TIMERS; i++) {
        list_add_tail(&timerarray[i].list, &free_timers);
    }
}

/* 
 * Allocates a timer struct from the free_timers list
 */
timer_t *allocTimer() {
    if (list_empty(&free_timers)) {
        LOG(ERROR, "Fatal: unable to allocate timer");
        return NULL;
    }
    else {
        timer_t *block;
        block = container_of(free_timers.next, timer_t, list);
        list_del(free_timers.next);
        memset(block, 0, sizeof(block)); /*setta tutti i campi di pcb a 0 (NULL) */
        return (block);
    }
}

/* 
 * Adds p to the list of free_timers
 */
void freeTimer(timer_t *p) { list_add_tail(&p->list, &free_timers); }

/* 
 * Adds a new timer to the list pointed by q
 */
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

/* 
 * Removes the first timer from the list pointed by q
 */
timer_t *removeTimer(struct list_head *q) {
    q = (struct list_head*)((uint64_t)q & 0x0000ffffffffffff);
    if (q == NULL || list_empty(q))
        return NULL;
    timer_t *first;
    first = container_of(q->next, typeof(*first), list);
    list_del(q->next);
    return first;
}

/* 
 * Removes any timer of the specified type from the list pointed by q
 */
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

/* 
 * Returns the first timer from the list pointed by q, without removing it
 */
timer_t *headTimer(struct list_head *q) {
    q = (struct list_head*)((uint64_t)q & 0x0000ffffffffffff);
    if (q == NULL || list_empty(q))
        return NULL;
    q = q->next;
    return container_of(q, timer_t, list);
}

/* 
 * Adds a new timer of the specified type
 */
void add_timer(uint64_t time, TIMER_TYPE type, int code) {
    timer_t *timer;
    
    removeTimerType(&pending_timers, type, code);
    timer = allocTimer();

    if (timer == NULL) {
        LOG(ERROR, "Fatal: no more timers!");
        return;
    }

    timer->time = time;
    timer->type = type;
    timer->code = code;

    insertTimer(&pending_timers, timer);
}

/* 
 * Initializes next with the first timer on the list
 */
int next_timer(timer_t *next) {
    timer_t *timer = headTimer(&pending_timers);
    if (timer == NULL)
        return -1;

    memcpy(next, timer, sizeof(timer_t));
    return 0; 
}

/* 
 * Initializes next with the first timer that has yet to expire
 */
int next_pending_timer(uint64_t currentTime, timer_t *next) {
    timer_t *timer = headTimer(&pending_timers);
    if (timer == NULL)
        return -1;

    memcpy(next, timer, sizeof(timer_t));
    if (timer->time < currentTime) {
        removeTimer(&pending_timers);
        freeTimer(timer);
        return 1;
    }
    return 0;
}