#ifndef __TYPES_H__
#define __TYPES_H__

typedef struct _tape_device {
    unsigned int status;
    unsigned int command;
    unsigned int data0;
    unsigned int data1;
    unsigned int mailbox;
} tapereg_t __attribute__((aligned(16)));

typedef struct _disk_device {
    unsigned int status;
    unsigned int command;
    unsigned int data0;
    unsigned int data1;
    unsigned int mailbox;
} diskreg_t __attribute__((aligned(16)));

typedef struct _terminal_device {
    unsigned int recv_status;
    unsigned int recv_command;
    unsigned int transm_status;
    unsigned int transm_command;
} termreg_t;

typedef struct _printer_device {
    unsigned int status;
    unsigned int command;
    unsigned int data0;
    unsigned int data1;
    unsigned int mailbox;
} printreg_t __attribute__((aligned(16)));


#endif