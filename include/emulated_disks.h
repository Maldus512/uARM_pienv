#ifndef __EMULATED_DISKS_H__
#define __EMULATED_DISKS_H__

#include "arch.h"
#include "types.h"

#define RESET           0
#define ACK             1
#define READ_REGISTERS  2
#define SEEKCYL         3
#define READBLK         4
#define WRITEBLK        5

#define DEVICE_NOT_INSTALLED    0
#define DEVICE_READY            1
#define ILLEGAL_OPERATION       2
#define DEVICE_BUSY             3
#define SEEK_ERROR              4
#define READ_ERROR              5
#define WRITE_ERROR             6
#define DMA_TRANSFER_ERROR      7

#define MAX_DISKS       4
#define MAX_CYLINDERS   0xFFFF
#define MAX_HEADS       0xFF
#define MAX_SECTORS     0xFF

#define DISKFILEID    0x0053504D
#define DISK_BLOCKSIZE  4096

#define GET_COMMAND(x)          (x & 0xFF)
#define GET_COMMAND_SECTNUM(x)  ((x >> 8) & 0xFF)
#define GET_COMMAND_HEADNUM(x)  ((x >> 16) & 0xFF)
#define GET_COMMAND_CYLNUM(x)   ((x >> 16) & 0xFFFF)

typedef struct {
    unsigned int cylinders;
    unsigned int heads;
    unsigned int sectors;
    unsigned int current_cylinder;
    unsigned int current_head;
    unsigned int current_sector;
    unsigned int rpm;
    unsigned int seektime;
    unsigned int dataoccupation;
    unsigned int fat32_cluster;
    diskreg_t *mailbox_registers;
    diskreg_t internal_registers;
} disk_internal_state_t;

extern disk_internal_state_t emulated_disks[MAX_DISKS];

void init_emulated_disks();
void manage_emulated_disk(int i);
void emulated_disk_mailbox(int i, diskreg_t *registers);

#endif