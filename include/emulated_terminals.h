#ifndef __EMULATED_DEVICES_H__
#define __EMULATED_DEVICES_H__

#include "arch.h"
#include "types.h"

#define MAX_TERMINALS 4

#define DEVICE_NOT_INSTALLED    0
#define DEVICE_READY            1
#define ILLEGAL_OP_CODE         2
#define DEVICE_BUSY             3
#define RECV_ERROR              4
#define TRANSIM_ERROR           4
#define CHAR_RECV               5
#define CHAR_TRANSMIT           5

#define RESET                   0
#define ACK                     1
#define TRANSMIT_CHAR           2
#define RECEIVE_CHAR            2

void lfb_init();
void lfb_send(int x, int y, char c);
void lfb_print(int x, int y, char *s);
void terminal_send(int i, char c);
void init_emulated_terminals();
extern volatile termreg_t terminals[MAX_TERMINALS];


#endif