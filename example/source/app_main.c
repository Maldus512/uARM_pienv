#include <stdint.h>
#include "bios_const.h"
#include "system.h"
#include "timers.h"
#include "uart.h"
#include "libuarm.h"
#include "arch.h"
#include "emulated_devices.h"
#include "interrupts.h"

#define INTERRUPT_HANDLER 0x7FFF0

#define ST_READY           1
#define ST_BUSY            3
#define ST_TRANSMITTED     5

#define CMD_ACK            1
#define CMD_TRANSMIT       2

#define CHAR_OFFSET        8
#define TERM_STATUS_MASK   0xFF

extern void uart0_puts(char *str);
static void term_puts(const char *str);


void copy_state(state_t *dest, state_t *src) {
    int i = 0;

    for (i = 0; i < 30; i++) {
        dest->general_purpose_registers[i] = src->general_purpose_registers[i];
    }
    dest->frame_pointer           = src->frame_pointer;
    dest->link_register           = src->link_register;
    dest->stack_pointer           = src->stack_pointer;
    dest->exception_link_register = src->exception_link_register;
    dest->TTBR0                   = src->TTBR0;
    dest->status_register         = src->status_register;
}

void test1() {
    int numeri[100];
    int contatore = 0;
    int level     = SYSCALL(SYS_GETCURRENTEL, 0, 0, 0);
    uart0_puts("livello 1: ");
    hexstring(level);
    while (1) {
        contatore         = (contatore + 1) % 100;
        numeri[contatore] = contatore;
        uart0_puts("test1 vivo: ");
        term_puts("test1 vivoaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        hexstring(numeri[contatore]);
        delay_us(1000* 1000);
    }
}

void test2() {
    int numeri[100];
    int contatore = 0;
    int level     = SYSCALL(SYS_GETCURRENTEL, 0, 0, 0);
    uart0_puts("livello 2: ");
    hexstring(level);
    while (1) {
        contatore         = (contatore + 1) % 70;
        numeri[contatore] = contatore;
        uart0_puts("test2 vivo: ");
        term_puts("test2 vivo\n");
        hexstring(numeri[contatore]);
        WAIT();
        delay_us(1000 * 1000);
    }
}
state_t  t1, t2;
state_t *current = 0;

extern volatile unsigned char __EL1_stack, __EL0_stack;

void interrupt() {
    state_t *oldarea = (state_t *)INTERRUPT_OLDAREA;
    copy_state(current, oldarea);
    setNextTimer(1000);

    if (current == &t1) {
        current = &t2;
    } else {
        current = &t1;
    }
    LDST(current);
}

static int term_putchar(char c);
static uint32_t tx_status(termreg_t *tp);

static termreg_t *term0_reg;


static void term_puts(const char *str)
{
    while (*str)
        if (term_putchar(*str++) == -1) {
            return;
        }
}

static int term_putchar(char c) 
{
    uint32_t stat;

    stat = tx_status(term0_reg);
    if (stat != ST_READY && stat != ST_TRANSMITTED) {
        return -1;
    }

    term0_reg->transm_command = ((c << CHAR_OFFSET) | CMD_TRANSMIT);

    //while ((stat = tx_status(term0_reg)) == ST_READY) ;
    WAIT();

    while ((stat = tx_status(term0_reg)) == ST_BUSY)
        ;

    term0_reg->transm_command = CMD_ACK;

    //while(term0_reg->transm_command == CMD_ACK) ;
    WAIT();

    if (stat != ST_TRANSMITTED) {
        return -1;
    }
    else
        return 0; 
}

static uint32_t tx_status(termreg_t *tp)
{
    return ((tp->transm_status) & TERM_STATUS_MASK);
}

int main() {
    term0_reg = (termreg_t *) DEV_REG_ADDR(IL_TERMINAL, 3);
    uart0_puts("sono l'applicazione\n");
    hexstring(SYSCALL(SYS_GETCURRENTSTATUS, 0, 0, 0));
    *((uint64_t *)INTERRUPT_HANDLER) = (uint64_t *)&interrupt;

    STST(&t1);
    STST(&t2);

    t1.exception_link_register = (uint64_t)test1;
    t2.exception_link_register = (uint64_t)test2;
    t1.stack_pointer           = (uint64_t)&__EL0_stack - 0x2000;
    t2.stack_pointer           = (uint64_t)&__EL0_stack - 0x4000;
    t1.status_register         = 0x340;
    t2.status_register         = 0x340;
    current                    = &t1;
    setNextTimer(1000);
    LDST(current);

    while (1) {
    }

    return 0;
}