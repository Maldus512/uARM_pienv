#ifndef __EMULATED_DEVICES_H__
#define __EMULATED_DEVICES_H__

#include "arch.h"
#include "types.h"

#define MAX_TERMINALS 4
#define MAX_PRINTERS 4

#define DEVICE_NOT_INSTALLED    0
#define DEVICE_READY            1
#define ILLEGAL_OPERATION       2
#define DEVICE_BUSY             3
#define RECV_ERROR              4
#define PRINT_ERROR             4
#define TRANSIM_ERROR           4
#define CHAR_RECV               5
#define CHAR_TRANSMIT           5

#define RESET                   0
#define ACK                     1
#define READ_REGISTERS          2
#define PRINT_CHAR              3

typedef struct {
    unsigned int executing_command;
    printreg_t *mailbox_registers;
    printreg_t internal_registers;
} printer_internal_state_t;

void lfb_init();
void lfb_send(int x, int y, char c);
void lfb_print(int x, int y, char *s);
void manage_emulated_printer(int i);
void emulated_printer_mailbox(int i, printreg_t *registers);
void init_emulated_printers();
extern volatile termreg_t terminals[MAX_TERMINALS];


#endif