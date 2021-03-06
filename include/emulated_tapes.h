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
#define READ_REGISTERS 2
#define SKIPBLK 3
#define READBLK 4
#define BACKBLK 5

#define DEVICE_NOT_INSTALLED    0
#define DEVICE_READY            1
#define ILLEGAL_OPERATION       2
#define DEVICE_BUSY             3
#define SKIP_ERROR              4
#define READ_ERROR              5
#define BACK_BLOCK_ERROR        6
#define DMA_TRANSFER_ERROR      7

#define MAX_TAPES   4

#define TAPEFILEID    0x0153504D
#define TAPE_BLOCKSIZE  4096

typedef struct {
    unsigned int block_index;
    unsigned int fat32_cluster;
    tapereg_t *mailbox_registers;
    tapereg_t internal_registers;
} tape_internal_state_t;

extern tape_internal_state_t emulated_tapes[MAX_TAPES];

void init_emulated_tapes();
void manage_emulated_tape(int i);
void emulated_tape_mailbox(int i, tapereg_t *registers);

#endif