#ifndef __EMULATED_TAPES_H__
#define __EMULATED_TAPES_H__

#include "arch.h"
#include "types.h"

#define EOT     0
#define EOF     1
#define EOB     2
#define TS      3

#define RESET   0
#define ACK     1
#define SKIPBLK 2
#define READBLK 3
#define BACKBLK 4
#define WRITEBLK 5

#define DEVICE_NOT_INSTALLED    0
#define DEVICE_READY            1
#define ILLEGAL_OPERATION       2
#define DEVICE_BUSY             3
#define SKIP_ERROR              4
#define READ_ERROR              5
#define BACK_BLOCK_ERROR        6
#define DMA_TRANSFER_ERROR      7

#define MAX_TAPES   4

typedef struct {
    unsigned int old_command;
    unsigned int executing_command;
    unsigned int block_index;
    unsigned int fat32_cluster;
    tapereg_t *device_registers;
} tape_internal_state_t;

extern volatile tape_internal_state_t emulated_tapes[MAX_TAPES];

unsigned int read_tape_block(int tape, unsigned char *buffer);
unsigned int write_tape_block(int tape, unsigned char *buffer);
void init_emulated_tapes();
void manage_emulated_tape(int i);

#endif