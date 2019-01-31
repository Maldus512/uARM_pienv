/*
 * Hardware Abstraction Layer for Raspberry Pi 3
 *
 * Copyright (C) 2018 Mattia Maldini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/******************************************************************************
 * This module contains uMPS2 TAPE emulation routines
 ******************************************************************************/

#include "uart.h"
#include "utils.h"
#include "emulated_tapes.h"
#include "sd.h"
#include "fat.h"
#include "interrupts.h"
#include "emulated_timers.h"

tape_internal_state_t emulated_tapes[MAX_TAPES];

/*
 * Initializes emulated tapes internal data structures
 */
void init_emulated_tapes() {
    int          i;
    char         nome[] = "TAPEn      ";
    char         string[64];
    unsigned int tapefileid;
    uint8_t *    deviceinstalled = (uint8_t *)DEVICE_INSTALLED;
    uint8_t      tmp;

    deviceinstalled[IL_TAPE] = 0x00;
    for (i = 0; i < MAX_TAPES; i++) {
        nome[4]                         = '0' + i;
        emulated_tapes[i].fat32_cluster = fat_getcluster(nome);
        emulated_tapes[i].block_index   = 0;

        if (emulated_tapes[i].fat32_cluster) {
            fat_readfile(emulated_tapes[i].fat32_cluster, (unsigned char *)&tapefileid, 0, 4);
            if (tapefileid != TAPEFILEID) {
                emulated_tapes[i].internal_registers.status = DEVICE_NOT_INSTALLED;
                emulated_tapes[i].internal_registers.data1  = EOT;
                strcpy(string, "Invalid tape file ");
                strcpy(&string[strlen(string)], nome);
                LOG(WARN, string);
            } else {
                strcpy(string, "Found tape device ");
                strcpy(&string[strlen(string)], nome);
                LOG(INFO, string);
                tmp                                         = deviceinstalled[IL_TAPE] | ((uint8_t)(1 << i));
                deviceinstalled[IL_TAPE]                    = tmp;
                emulated_tapes[i].internal_registers.status = DEVICE_READY;
                emulated_tapes[i].internal_registers.data1  = TS;
            }
        } else {
            emulated_tapes[i].internal_registers.status = DEVICE_NOT_INSTALLED;
            emulated_tapes[i].internal_registers.data1  = EOT;
        }
        emulated_tapes[i].internal_registers.command = RESET;
    }
}

/*
 * Update the tape DATA1 register with the marker found at the end of the current block
 */
int update_tape_head(int tape) {
    unsigned int head, seek;
    if (emulated_tapes[tape].block_index == 0) {
        emulated_tapes[tape].internal_registers.data1 = TS;
    } else {
        seek = emulated_tapes[tape].block_index * TAPE_BLOCKSIZE + emulated_tapes[tape].block_index * 4;
        fat_readfile(emulated_tapes[tape].fat32_cluster, (unsigned char *)&head, seek, 4);

        if (head != EOB && head != EOF && head != EOT) {
            return -1;
        }
        emulated_tapes[tape].internal_registers.data1 = head;
    }
    return 0;
}

/*
 * Reads the 4KiB block found at the current block index of the tape
 */
int read_tape_block(int tape, unsigned char *buffer) {
    unsigned int block_start;
    if (emulated_tapes[tape].fat32_cluster == 0)
        return 0;
    /* The block starts at index*4K (size of blocks) + (index+1)*4 (end of block indicator size, including its tape file
     * id) */
    block_start = emulated_tapes[tape].block_index * TAPE_BLOCKSIZE + (emulated_tapes[tape].block_index + 1) * 4;

    return fat_readfile(emulated_tapes[tape].fat32_cluster, buffer, block_start, TAPE_BLOCKSIZE);
    // return fat_transferfile(emulated_tapes[tape].fat32_cluster, buffer, emulated_tapes[tape].block_index,
    // SD_READBLOCK);
}

/*
 * Writes the 4KiB block found at the current block index of the tape
 */
unsigned int write_tape_block(int tape, unsigned char *buffer) {
    if (emulated_tapes[tape].fat32_cluster == 0)
        return 0;
    return fat_transferfile(emulated_tapes[tape].fat32_cluster, buffer, emulated_tapes[tape].block_index,
                            SD_WRITEBLOCK);
}

/*
 * Moves the tape head to the next block (if present)
 */
int tape_next_block(int tape) {
    if (emulated_tapes[tape].internal_registers.data1 == EOT)
        return -1;

    emulated_tapes[tape].block_index++;
    return update_tape_head(tape);
}

/*
 * Moves the tape head to the previous block (if present)
 */
int tape_prev_block(int tape) {
    if (emulated_tapes[tape].block_index > 0) {
        emulated_tapes[tape].block_index--;
        return update_tape_head(tape);
    } else {
        return -1;
    }
}

/*
 *  Routine executing commands found in registers for tape i. To be called at an interval
 */
void manage_emulated_tape(int i) {
    uint8_t *interruptlines = (uint8_t *)INTERRUPT_LINES;
    uint8_t  intline;

    if (emulated_tapes[i].internal_registers.status == DEVICE_NOT_INSTALLED)
        return;

    if (emulated_tapes[i].internal_registers.status == DEVICE_BUSY) {
        switch (emulated_tapes[i].internal_registers.command) {
            case SKIPBLK:
                if (tape_next_block(i) < 0) {
                    emulated_tapes[i].internal_registers.status = SKIP_ERROR;
                } else {
                    emulated_tapes[i].internal_registers.status = DEVICE_READY;
                }
                memcpy(emulated_tapes[i].mailbox_registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                emulated_tapes[i].mailbox_registers = NULL;
                intline                             = interruptlines[IL_TAPE] | ((uint8_t)(1 << i));
                interruptlines[IL_TAPE]             = intline;
                break;

            case BACKBLK:
                if (tape_prev_block(i) < 0) {
                    emulated_tapes[i].internal_registers.status = BACK_BLOCK_ERROR;
                } else {
                    emulated_tapes[i].internal_registers.status = DEVICE_READY;
                }
                memcpy(emulated_tapes[i].mailbox_registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                emulated_tapes[i].mailbox_registers = NULL;
                intline                             = interruptlines[IL_TAPE] | ((uint8_t)(1 << i));
                interruptlines[IL_TAPE]             = intline;
                break;

            case READBLK:
                if (read_tape_block(i, (unsigned char *)(uint64_t)emulated_tapes[i].internal_registers.data0) < 0) {
                    emulated_tapes[i].internal_registers.status = READ_ERROR;
                } else {
                    emulated_tapes[i].block_index++;
                    if (update_tape_head(i) < 0) {
                        emulated_tapes[i].internal_registers.status = READ_ERROR;
                    } else {
                        emulated_tapes[i].internal_registers.status = DEVICE_READY;
                    }
                    intline                 = interruptlines[IL_TAPE] | ((uint8_t)(1 << i));
                    interruptlines[IL_TAPE] = intline;
                }
                memcpy(emulated_tapes[i].mailbox_registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                emulated_tapes[i].mailbox_registers = NULL;
                break;

            default:
                break;
        }
    }
}



/*
 *  Routine managing the immediate command sent to a tape through a mailbox
 */
void emulated_tape_mailbox(int i, tapereg_t *registers) {
    uint8_t *interruptlines = (uint8_t *)INTERRUPT_LINES;
    uint8_t  intline;

    if (registers == NULL)
        return;

    emulated_tapes[i].internal_registers.mailbox = 1;

    if (emulated_tapes[i].internal_registers.status == DEVICE_NOT_INSTALLED && registers->command != READ_REGISTERS) {
        memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
        return;
    }


    switch (registers->command) {
        case RESET:
            emulated_tapes[i].internal_registers.command = RESET;
            emulated_tapes[i].internal_registers.status  = DEVICE_READY;
            emulated_tapes[i].block_index                = 0;
            update_tape_head(i);
            intline                 = interruptlines[IL_TAPE] | ((uint8_t)(1 << i));
            interruptlines[IL_TAPE] = intline;
            memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
            break;

        case ACK:
            if (emulated_tapes[i].internal_registers.status != DEVICE_BUSY) {
                emulated_tapes[i].internal_registers.command = ACK;
                emulated_tapes[i].internal_registers.status  = DEVICE_READY;
                intline                                      = interruptlines[IL_TAPE] & ((uint8_t)(~(1 << i)));
                interruptlines[IL_TAPE]                      = intline;
            }
            memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
            break;

        case READ_REGISTERS:
            memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
            break;

        case SKIPBLK:
            if (emulated_tapes[i].internal_registers.status != DEVICE_BUSY) {
                emulated_tapes[i].internal_registers.command = SKIPBLK;
                emulated_tapes[i].internal_registers.status  = DEVICE_BUSY;
                emulated_tapes[i].mailbox_registers          = registers;
                memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                set_device_timer(1000, TAPE, i);
            } else {
                memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
            }
            break;

        case BACKBLK:
            if (emulated_tapes[i].internal_registers.status != DEVICE_BUSY) {
                emulated_tapes[i].internal_registers.command = BACKBLK;
                emulated_tapes[i].internal_registers.status  = DEVICE_BUSY;
                emulated_tapes[i].mailbox_registers          = registers;
                memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                set_device_timer(1000, TAPE, i);
            } else {
                memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
            }
            break;

        case READBLK:
            if (emulated_tapes[i].internal_registers.status != DEVICE_BUSY) {
                if (emulated_tapes[i].internal_registers.data1 == EOT) {
                    emulated_tapes[i].internal_registers.status = READ_ERROR;
                    registers->mailbox                          = 2;
                    memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                } else {
                    emulated_tapes[i].internal_registers.command = READBLK;
                    emulated_tapes[i].internal_registers.data0   = registers->data0;
                    emulated_tapes[i].internal_registers.status  = DEVICE_BUSY;
                    emulated_tapes[i].mailbox_registers          = registers;
                    memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
                    set_device_timer(1000, TAPE, i);
                }
            } else {
                memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
            }
            break;

        default:
            emulated_tapes[i].internal_registers.status = ILLEGAL_OPERATION;
            memcpy(registers, &emulated_tapes[i].internal_registers, sizeof(tapereg_t));
            break;
    }
}