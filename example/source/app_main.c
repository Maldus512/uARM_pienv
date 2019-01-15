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
#define READ_REGISTERS 6

#define RESET 0
#define ACK 1
#define PRINT_CHAR 2

#define RESET 0
#define ACK 1
#define READBLK 3
#define WRITEBLK 5

static void term_puts(const char *str);

state_t  t1, t2;
state_t *current;
int      semaforo = 0;


void delay(unsigned int us) {
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

static uint32_t print_status(printreg_t *tp) { return ((tp->status) & TERM_STATUS_MASK); }

static int term_putchar(char c) {
    uint32_t   stat;
    printreg_t print0_reg = {0, READ_REGISTERS, 0, 0, 0};

    CORE0_MAILBOX0 = (((uint32_t)(uint64_t)&print0_reg) & ~0xF) | 0x0;
    while (print0_reg.mailbox == 0)
        nop();

    stat = print_status(&print0_reg);
    if (stat != ST_READY)
        return -1;

    print0_reg.mailbox = 0;
    print0_reg.command = PRINT_CHAR;
    print0_reg.data0   = c;
    CORE0_MAILBOX0     = (((uint32_t)(uint64_t)&print0_reg) & ~0xF) | 0x0;
    while (print0_reg.mailbox == 0)
        nop();
    while (print_status(&print0_reg) == ST_BUSY)
        nop();

    print0_reg.mailbox = 0;
    print0_reg.command = CMD_ACK;
    CORE0_MAILBOX0     = (((uint32_t)(uint64_t)&print0_reg) & ~0xF) | 0x0;
    while (print0_reg.mailbox == 0)
        nop();

    return 0;
}

static void term_puts(const char *str) {
    while (*str)
        if (term_putchar(*str++) == -1)
            return;
}

static void uart0_putc(char c) {
    unsigned int *DATA = (unsigned int *)0x3F201000;
    unsigned int *FLAG = (unsigned int *)(0x3F201000 + 24);
    while (*FLAG & 0x20)
        ;
    *DATA = c;
}

static void print(char *s) {
    while (*s) {
        /* convert newline to carrige return + newline */
        if (*s == '\n')
            uart0_putc('\r');
        uart0_putc(*s++);
    }
}

static void uart_hex(unsigned int d) {
    unsigned int n;
    int          c;
    for (c = 28; c >= 0; c -= 4) {
        n = (d >> c) & 0xF;
        n += n > 9 ? 0x37 : 0x30;
        uart0_putc(n);
    }
}

void test1() {
    int           numeri[100];
    unsigned char buffer[512];
    tapereg_t     tape_reg = {0, READ_REGISTERS, 0, 0, 0};

    int contatore = 0;
    print("partenza 1\n");

    CORE0_MAILBOX0 = (((uint32_t)(uint64_t)&tape_reg) & ~0xF) | 0x4;
    while (tape_reg.mailbox == 0)
        nop();

    tape_reg.data0   = (unsigned int)(unsigned long)buffer;
    tape_reg.command = READBLK;
    CORE0_MAILBOX0   = (((uint32_t)(uint64_t)&tape_reg) & ~0xF) | 0x4;
    while (semaforo == 0)
        ;

    print("letto nastro: ");
    print((char *)buffer);
    print("\n");

    buffer[0] = buffer[0] == 'M' ? 'P' : 'M';

    semaforo         = 0;
    tape_reg.command = WRITEBLK;
    CORE0_MAILBOX0   = (((uint32_t)(uint64_t)&tape_reg) & ~0xF) | 0x4;
    while (semaforo == 0)
        ;

    print("scritto nastro\n");

    while (1) {
        contatore         = (contatore + 1) % 100;
        numeri[contatore] = contatore;
        print("test1 vivo: ");
        term_puts("test1 vivo");
        uart_hex(numeri[contatore]);
        print("\n");
        delay(600 * 1000);
        // WAIT();
    }
}

void test2() {
    unsigned long timer;
    print("partenza 2\n");
    while (1) {
        timer = get_us();
        print("test2 vivo: ");
        SYSCALL(5, 0, 0, 0);
        term_puts("test2 vivo\n");
        uart_hex(timer);
        print("\n");
        delay(1000 * 1000);
    }
}

void synchronous(unsigned int code, unsigned int x0, unsigned int x1, unsigned int x2) { print("system call!\n"); }

void interrupt() {
    state_t * oldarea         = (state_t *)INTERRUPT_OLDAREA;
    uint8_t * interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    tapereg_t tape_reg        = {0, ACK, 0, 0, 0};

    copy_state(current, oldarea);

    if (interrupt_lines[IL_TIMER]) {
        /* This also clears pending interrupts */
        set_next_timer(10 * 1000);

        if (current == &t1) {
            current = &t2;
        } else if (current == &t2) {
            current = &t1;
        }
    }

    if (interrupt_lines[IL_TAPE]) {
        semaforo = 1;

        CORE0_MAILBOX0 = (((uint32_t)(uint64_t)&tape_reg) & ~0xF) | 0x4;
        while (tape_reg.mailbox == 0)
            nop();
    }

    LDST(current);
}

int main() {
    *((uint8_t *)INTERRUPT_MASK)       = 0xFC;     //&= ~((1 << IL_TIMER) | (1 << IL_TAPE));
    *((uint64_t *)INTERRUPT_HANDLER)   = (uint64_t)&interrupt;
    *((uint64_t *)SYNCHRONOUS_HANDLER) = (uint64_t)&synchronous;

    print("sono l'applicazione\n");

    STST(&t1);
    STST(&t2);

    t1.exception_link_register = (uint64_t)test1;
    t2.exception_link_register = (uint64_t)test2;
    t1.stack_pointer           = (uint64_t)0x1000000 + 0x2000;
    t2.stack_pointer           = (uint64_t)0x1000000 + 0x4000;
    t1.status_register         = 0x300;
    t2.status_register         = 0x300;
    current                    = &t2;
    set_next_timer(1000);

    print("about to launch the first process\n");
    LDST(current);
    return 0;
}