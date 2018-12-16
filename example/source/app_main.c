#include <stdint.h>
#include "arch.h"
#include "bios_const.h"
#include "libuarm.h"
#include "types.h"

#define ST_READY 1
#define ST_BUSY 3
#define ST_TRANSMITTED 5

#define CMD_ACK 1
#define CMD_TRANSMIT 2

#define CHAR_OFFSET 8
#define TERM_STATUS_MASK 0xFF

#define DEVICE_READY 1
#define DEVICE_BUSY 3
#define CHAR_TRANSMIT 5

#define RESET 0
#define ACK 1
#define TRANSMIT_CHAR 2

#define RESET 0
#define ACK 1
#define READBLK 3


extern void hexstring(unsigned int);
static void term_puts(const char *str);

state_t  t1, t2;
state_t *current = 0;

extern volatile unsigned char __EL1_stack, __EL0_stack;

extern void delay_us(unsigned int);

static int      term_putchar(char c);
static uint32_t tx_status(termreg_t *tp);

void mydelay2(unsigned int us) {
    volatile unsigned long timestamp = get_us();
    volatile unsigned long end       = timestamp + us;

    while ((unsigned long)timestamp < (unsigned long)(end)) {
        timestamp = get_us();
    }
}


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
    int           numeri[100];
    unsigned char buffer[512];

    tapereg_t *tape      = DEV_REG_ADDR(IL_TAPE, 0);
    int        contatore = 0;
    tprint("partenza 1\n");
    tape->data0   = (unsigned int)(unsigned long)buffer;
    tape->command = READBLK;
    WAIT();
    while (tape->status == DEVICE_BUSY)
        ;

    tprint("letto nastro: ");
    buffer[32] = '\0';
    tprint((char *)buffer);
    tprint("\n");
    while (1) {
        contatore         = (contatore + 1) % 100;
        numeri[contatore] = contatore;
        tprint("test1 vivo: ");
        term_puts("test1 vivoaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        hexstring(numeri[contatore]);
        mydelay2(1000 * 1000);
    }
}

void test2() {
    int           numeri[100];
    int           contatore = 0;
    unsigned long timer;
    tprint("partenza 2\n");
    while (1) {
        timer             = get_us();
        contatore         = (contatore + 1) % 70;
        numeri[contatore] = contatore;
        tprint("test2 vivo: ");
        term_puts("test2 vivo\n");
        hexstring(timer);
        mydelay2(1000 * 1000);
    }
}

void interrupt() {
    state_t *oldarea = (state_t *)INTERRUPT_OLDAREA;

    copy_state(current, oldarea);
    set_next_timer(1000*10);

    if (current == &t1) {
        current = &t2;
    } else {
        current = &t1;
    }
    LDST(current);
}


static void term_puts(const char *str) {
    while (*str)
        if (term_putchar(*str++) == -1) {
            return;
        }
}

static int term_putchar(char c) {
    uint32_t   stat;
    termreg_t *term0_reg = (termreg_t *)DEV_REG_ADDR(IL_TERMINAL, 3);

    stat = tx_status(term0_reg);
    if (stat != ST_READY && stat != ST_TRANSMITTED) {
        return -1;
    }

    term0_reg->transm_command = ((c << CHAR_OFFSET) | CMD_TRANSMIT);
    WAIT();

    while ((stat = tx_status(term0_reg)) == ST_BUSY)
        ;

    term0_reg->transm_command = CMD_ACK;
    WAIT();

    if (stat != ST_TRANSMITTED) {
        return -1;
    } else
        return 0;
}

static uint32_t tx_status(termreg_t *tp) { return ((tp->transm_status) & TERM_STATUS_MASK); }

int main() {
    *((uint8_t *)INTERRUPT_MASK) &= ~(1 << IL_TIMER);
    tprint("sono l'applicazione\n");
    *((uint64_t *)INTERRUPT_HANDLER) = (uint64_t)&interrupt;

    STST(&t1);
    STST(&t2);

    t1.exception_link_register = (uint64_t)test1;
    t2.exception_link_register = (uint64_t)test2;
    t1.stack_pointer           = (uint64_t)&__EL0_stack - 0x2000;
    t2.stack_pointer           = (uint64_t)&__EL0_stack - 0x4000;
    t1.status_register         = 0x340;
    t2.status_register         = 0x340;
    current                    = &t1;
    set_next_timer(1000);
    LDST(current);

    return 0;
}