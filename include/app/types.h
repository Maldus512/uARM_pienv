#ifndef __TYPES_H__
#define __TYPES_H__

typedef struct _tape_device {
    unsigned int status;
    unsigned int command;
    unsigned int data0;
    unsigned int data1;
} tapereg_t;

typedef struct _terminal_device {
    unsigned int recv_status;
    unsigned int recv_command;
    unsigned int transm_status;
    unsigned int transm_command;
} termreg_t;

#endif