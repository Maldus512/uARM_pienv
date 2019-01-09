#include "uart.h"
#include "utils.h"
#include "emulated_tapes.h"
#include "sd.h"
#include "fat.h"
#include "timers.h"

tape_internal_state_t emulated_tapes[MAX_TAPES];

void init_emulated_tapes() {
    int      i;
    char     nome[] = "TAPEn      ";
    char     string[32];
    uint8_t *device_installed = (uint8_t *)DEVICE_INSTALLED;
    uint8_t  tmp;

    device_installed[IL_TAPE] = 0x00;
    for (i = 0; i < MAX_TAPES; i++) {
        nome[4]                         = '0' + i;
        emulated_tapes[i].fat32_cluster = fat_getcluster(nome);
        emulated_tapes[i].block_index   = 0;

        if (emulated_tapes[i].fat32_cluster) {
            strcpy(string, "Found tape device ");
            strcpy(&string[strlen(string)], nome);
            LOG(INFO, string);
            tmp                                         = device_installed[IL_TAPE] | ((uint8_t)(1 << i));
            device_installed[IL_TAPE]                   = tmp;
            emulated_tapes[i].internal_registers.status = DEVICE_READY;
            emulated_tapes[i].internal_registers.data1  = TS;
        } else {
            emulated_tapes[i].internal_registers.status = DEVICE_NOT_INSTALLED;
            emulated_tapes[i].internal_registers.data1  = EOT;
        }
        emulated_tapes[i].internal_registers.command = RESET;
    }
}

unsigned int read_tape_block(int tape, unsigned char *buffer) {
    if (emulated_tapes[tape].fat32_cluster == 0)
        return 0;
    return fat_transferfile(emulated_tapes[tape].fat32_cluster, buffer, emulated_tapes[tape].block_index, SD_READBLOCK);
}

unsigned int write_tape_block(int tape, unsigned char *buffer) {
    if (emulated_tapes[tape].fat32_cluster == 0)
        return 0;
    return fat_transferfile(emulated_tapes[tape].fat32_cluster, buffer, emulated_tapes[tape].block_index,
                            SD_WRITEBLOCK);
}

void tape_next_block(int tape) { emulated_tapes[tape].block_index++; }

void tape_prev_block(int tape) {
    if (emulated_tapes[tape].block_index > 0)
        emulated_tapes[tape].block_index--;
}

void manage_emulated_tape(int i) {
    uint8_t *interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t  intline;

    if (emulated_tapes[i].internal_registers.status == DEVICE_NOT_INSTALLED)
        return;

    if (emulated_tapes[i].internal_registers.status == DEVICE_BUSY) {
        switch (emulated_tapes[i].internal_registers.command) {
            case SKIPBLK:
                tape_next_block(i);
                emulated_tapes[i].internal_registers.status = DEVICE_READY;
                memcpy(emulated_tapes[i].mailbox_registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                emulated_tapes[i].mailbox_registers = NULL;
                intline                             = interrupt_lines[IL_TAPE] | ((uint8_t)(1 << i));
                interrupt_lines[IL_TAPE]            = intline;
                break;
            case READBLK:
                if (read_tape_block(i, (unsigned char *)(uint64_t)emulated_tapes[i].internal_registers.data0) < 0) {
                    emulated_tapes[i].internal_registers.status = READ_ERROR;
                } else {
                    emulated_tapes[i].internal_registers.status = DEVICE_READY;
                    intline                                     = interrupt_lines[IL_TAPE] | ((uint8_t)(1 << i));
                    interrupt_lines[IL_TAPE]                    = intline;
                }
                memcpy(emulated_tapes[i].mailbox_registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                emulated_tapes[i].mailbox_registers = NULL;
                break;

            case WRITEBLK:
                if (write_tape_block(i, (unsigned char *)(uint64_t)emulated_tapes[i].internal_registers.data0) < 0) {
                    emulated_tapes[i].internal_registers.status = READ_ERROR;
                } else {
                    emulated_tapes[i].internal_registers.status = DEVICE_READY;
                    intline                                     = interrupt_lines[IL_TAPE] | ((uint8_t)(1 << i));
                    interrupt_lines[IL_TAPE]                    = intline;
                }
                memcpy(emulated_tapes[i].mailbox_registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                emulated_tapes[i].mailbox_registers = NULL;
                break;
            default:
                break;
        }
    }
}



void emulated_tape_mailbox(int i, tapereg_t *registers) {
    uint8_t *interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t  intline;

    if (registers == NULL)
        return;

    emulated_tapes[i].mailbox_registers = registers;

    if (emulated_tapes[i].internal_registers.status == DEVICE_NOT_INSTALLED && registers->command != READ_REGISTERS)
        return;

    switch (registers->command) {
        case RESET:
            emulated_tapes[i].internal_registers.command = RESET;
            emulated_tapes[i].internal_registers.status  = DEVICE_READY;
            break;
        case ACK:
            if (emulated_tapes[i].internal_registers.status != DEVICE_BUSY) {
                emulated_tapes[i].internal_registers.command = ACK;
                emulated_tapes[i].internal_registers.status  = DEVICE_READY;
                intline                                      = interrupt_lines[IL_TAPE] & ((uint8_t)(~(1 << i)));
                interrupt_lines[IL_TAPE]                     = intline;
            }
            break;
        case READ_REGISTERS:
            break;
        case SKIPBLK:
            if (emulated_tapes[i].internal_registers.status != DEVICE_BUSY) {
                emulated_tapes[i].internal_registers.command = SKIPBLK;
                emulated_tapes[i].internal_registers.status  = DEVICE_BUSY;
                setTimer(100);
            }
        case READBLK:
            if (emulated_tapes[i].internal_registers.status != DEVICE_BUSY) {
                emulated_tapes[i].internal_registers.command = READBLK;
                emulated_tapes[i].internal_registers.data0   = registers->data0;
                emulated_tapes[i].internal_registers.status  = DEVICE_BUSY;
                setTimer(100);
            }
            break;
        case WRITEBLK:
            if (emulated_tapes[i].internal_registers.status != DEVICE_BUSY) {
                emulated_tapes[i].internal_registers.command = WRITEBLK;
                emulated_tapes[i].internal_registers.data0   = registers->data0;
                emulated_tapes[i].internal_registers.status  = DEVICE_BUSY;
                setTimer(100);
            }
            break;
        default:
            emulated_tapes[i].internal_registers.status = ILLEGAL_OPERATION;
            break;
    }

    memcpy(emulated_tapes[i].mailbox_registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
    emulated_tapes[i].mailbox_registers->mailbox = 1;
}