#include <stdint.h>
#include "arch.h"
#include "bios_const.h"
#include "libuarm.h"
#include "types.h"
#include "mmu.h"

#define ST_READY 1
#define ST_BUSY 3
#define ST_TRANSMITTED 5

#define CMD_ACK 1
#define CMD_TRANSMIT 2

#define CHAR_OFFSET 8
#define TERM_STATUS_MASK 0xFF

#define DEVICE_NOT_INSTALLED 0
#define DEVICE_READY 1
#define DEVICE_BUSY 3
#define CHAR_TRANSMIT 5

#define RESET 0
#define ACK 1
#define READ_REGISTERS 2
#define PRINT_CHAR 3

#define RESET 0
#define ACK 1
#define SEEKCYL 3
#define READBLK 4
#define WRITEBLK 5

#define CORE1_MAILBOX0 (*(uint32_t *)0x40000090)
#define CORE1_MAILBOX0_CLEAR (*(uint32_t *)0x400000D0)

static void term_puts(const char *str);

state_t  t1, t2;
state_t *current;
int      semaforo = 0;


/* Level0 1:1 mapping to Level1 */
static __attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR app0map1to1[512] = {0};

/* This will have 1024 entries x 2M so a full range of 2GB */
static __attribute__((aligned(4096))) VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR app1map1to1[1024] = {0};


void delay(unsigned int us) {
    volatile unsigned long timestamp = getTOD();
    volatile unsigned long end       = timestamp + us;

    while ((unsigned long)timestamp < (unsigned long)(end)) {
        timestamp = getTOD();
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

void print(char *s) {
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

void testTape() {
    unsigned char buffer[4096];
    tapereg_t     tape_reg = {0, READ_REGISTERS, 0, 0, 0};
    do {
        semaforo         = 0;
        tape_reg.command = READ_REGISTERS;
        tape_reg.mailbox = 0;
        CORE0_MAILBOX0   = (((uint32_t)(uint64_t)&tape_reg) & ~0xF) | 0x4;
        while (tape_reg.mailbox == 0)
            nop();

        if (tape_reg.status == DEVICE_NOT_INSTALLED) {
            print("tape non trovata\n");
            break;
        }

        tape_reg.data0   = (unsigned int)(unsigned long)buffer;
        tape_reg.command = READBLK;
        CORE0_MAILBOX0   = (((uint32_t)(uint64_t)&tape_reg) & ~0xF) | 0x4;
        while (semaforo == 0)
            ;

        print("\n\nletto nastro: ");
        print((char *)buffer);
        print("\n");
    } while (tape_reg.data1 != 0);
}

void testDisk() {
    unsigned char buffer[4096];
    diskreg_t     disk_reg = {0, READ_REGISTERS, 0, 0, 0};

    semaforo         = 0;
    disk_reg.command = READ_REGISTERS;
    disk_reg.mailbox = 0;
    CORE0_MAILBOX0   = (((uint32_t)(uint64_t)&disk_reg) & ~0xF) | 0x8;
    while (disk_reg.mailbox == 0)
        nop();

    if (disk_reg.status == DEVICE_NOT_INSTALLED) {
        print("disco non trovato");
        return;
    }

    disk_reg.command = SEEKCYL | (1 << 16);
    semaforo         = 0;
    CORE0_MAILBOX0   = (((uint32_t)(uint64_t)&disk_reg) & ~0xF) | 0x8;
    while (semaforo == 0)
        ;

    disk_reg.data0   = (unsigned int)(unsigned long)buffer;
    disk_reg.command = READBLK | (1 << 8);
    semaforo         = 0;
    CORE0_MAILBOX0   = (((uint32_t)(uint64_t)&disk_reg) & ~0xF) | 0x8;
    while (semaforo == 0)
        ;

    print("\n\nletto disco: ");
    print((char *)buffer);
    print("\n");

    buffer[0] = buffer[0] == 'M' ? 'P' : 'M';

    disk_reg.data0   = (unsigned int)(unsigned long)buffer;
    disk_reg.command = WRITEBLK | (1 << 8);
    semaforo         = 0;
    CORE0_MAILBOX0   = (((uint32_t)(uint64_t)&disk_reg) & ~0xF) | 0x8;
    while (semaforo == 0)
        ;

    print("scritto disco\n");
}

void test1() {
    int numeri[100];
    int contatore = 0;
    print("partenza 1\n");

    testTape();
    testDisk();

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
    CORE1_MAILBOX0 = 1;
    while (1) {
        timer = getTOD();
        print("test2 vivo: ");
        SYSCALL(5, 0, 0, 0);
        term_puts("test2 vivo\n");
        uart_hex(timer);
        print("\n");
        delay(1000 * 1000);
    }
}

void synchronous(unsigned int code, unsigned int x0, unsigned int x1, unsigned int x2) {
    unsigned int core    = getCORE();
    state_t *    oldarea = (state_t *)(SYNCHRONOUS_OLDAREA + CORE_OFFSET * core);
    print("system call!\n");
    LDST(oldarea);
}

void interrupt() {
    state_t *    oldarea;
    uint8_t *    interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    tapereg_t    tape_reg        = {0, ACK, 0, 0, 0};
    diskreg_t    disk_reg        = {0, ACK, 0, 0, 0};
    unsigned int core;

    core = getCORE();

    if (core == 0) {
        oldarea = (state_t *)INTERRUPT_OLDAREA;
        copy_state(current, oldarea);

        if (interrupt_lines[IL_TIMER]) {
            /* This also clears pending interrupts */
            setTIMER(10 * 1000);

            if (current == &t1) {
                current = &t2;
            } else if (current == &t2) {
                current = &t1;
            }
        }

        if (interrupt_lines[IL_DISK]) {
            semaforo = 1;

            CORE0_MAILBOX0 = (((uint32_t)(uint64_t)&disk_reg) & ~0xF) | 0x8;
            while (disk_reg.mailbox == 0)
                nop();
        } else if (interrupt_lines[IL_TAPE]) {
            semaforo = 1;

            CORE0_MAILBOX0 = (((uint32_t)(uint64_t)&tape_reg) & ~0xF) | 0x4;
            while (tape_reg.mailbox == 0)
                nop();
        }

        LDST(current);
    } else {
        oldarea = (state_t *)(INTERRUPT_OLDAREA + CORE_OFFSET * core);
        nop();
        nop();
        nop();
        print("multicore!");
        CORE1_MAILBOX0_CLEAR = 0xFFFFFFFF;
        LDST(oldarea);
    }
}

int main() {
    char string[128];
    *((uint8_t *)INTERRUPT_MASK)       = 0xF9;
    *((uint64_t *)INTERRUPT_HANDLER)   = (uint64_t)&interrupt;
    *((uint64_t *)SYNCHRONOUS_HANDLER) = (uint64_t)&synchronous;
    *((uint64_t *)KERNEL_CORE0_SP)     = (uint64_t)0x1000000;
    *((uint64_t *)KERNEL_CORE1_SP)     = (uint64_t)0x1002000;
    *((uint64_t *)KERNEL_CORE2_SP)     = (uint64_t)0x1004000;
    *((uint64_t *)KERNEL_CORE3_SP)     = (uint64_t)0x1006000;

    print("sono l'applicazione\n");

    STST(&t1);
    STST(&t2);

    t1.exception_link_register = (uint64_t)test1;
    t2.exception_link_register = (uint64_t)test2;
    t1.stack_pointer           = (uint64_t)0x1006000 + 0x2000;
    t2.stack_pointer           = (uint64_t)0x1006000 + 0x4000;
    t1.status_register         = 0x300;
    t2.status_register         = 0x300;
    t1.TTBR0                   = ((uint64_t)((uint64_t)app0map1to1) | 0UL << 48);
    t2.TTBR0                   = ((uint64_t)((uint64_t)app0map1to1) | 0UL << 48);
    current                    = &t2;

    init_page_tables(app0map1to1, app1map1to1, APBITS_NO_LIMIT);

    print("about to launch the first process\n");
    setTIMER(1000);
    LDST(current);
    return 0;
}