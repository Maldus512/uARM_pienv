#include "uart.h"
#include "emulated_tapes.h"
#include "sd.h"
#include "fat.h"

volatile tape_internal_state_t emulated_tapes[MAX_TAPES];

void init_emulated_tapes() {
    int      i;
    char     nome[]           = "TAPEn      ";
    uint8_t *device_installed = (uint8_t *)DEVICE_INSTALLED;
    uint8_t  tmp;

    device_installed[IL_TAPE] = 0x00;
    for (i = 0; i < MAX_TAPES; i++) {
        emulated_tapes[i].old_command       = RESET;
        emulated_tapes[i].executing_command = RESET;
        emulated_tapes[i].device_registers  = (tapereg_t *)DEV_REG_ADDR(IL_TAPE, i);
        nome[4]                             = '0' + i;
        emulated_tapes[i].fat32_cluster     = fat_getcluster(nome);
        emulated_tapes[i].block_index       = 0;

        if (emulated_tapes[i].fat32_cluster) {
            emulated_tapes[i].device_registers->status = DEVICE_READY;
            uart0_puts("Found tape device ");
            uart0_puts(nome);
            uart0_puts("\n");
            tmp                                       = device_installed[IL_TAPE] | ((uint8_t)(1 << i));
            device_installed[IL_TAPE]                 = tmp;
            emulated_tapes[i].device_registers->data1 = TS;
        } else {
            emulated_tapes[i].device_registers->status = DEVICE_NOT_INSTALLED;
            emulated_tapes[i].device_registers->data1  = EOT;
        }
        emulated_tapes[i].device_registers->command = RESET;
    }
}

unsigned int read_tape_block(int tape, unsigned char *buffer) {
    if (emulated_tapes[tape].fat32_cluster == 0)
        return 0;
    return fat_transferfile(emulated_tapes[tape].fat32_cluster, buffer, emulated_tapes[tape].block_index++, SD_READBLOCK);
}

unsigned int write_tape_block(int tape, unsigned char *buffer) {
    if (emulated_tapes[tape].fat32_cluster == 0)
        return 0;
    return fat_transferfile(emulated_tapes[tape].fat32_cluster, buffer, emulated_tapes[tape].block_index++,
                            SD_WRITEBLOCK);
}

void tape_next_block(int tape) { emulated_tapes[tape].block_index++; }

void tape_prev_block(int tape) {
    if (emulated_tapes[tape].block_index > 0)
        emulated_tapes[tape].block_index--;
}

void manage_emulated_tape(int i) {
    tapereg_t *tape            = emulated_tapes[i].device_registers;
    uint8_t *  interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t    interrupt_mask  = *((uint8_t *)INTERRUPT_MASK);
    uint8_t    intline;

    if (tape->status == DEVICE_NOT_INSTALLED)
        return;

    if (emulated_tapes[i].executing_command != RESET) {
        switch (emulated_tapes[i].executing_command) {
            case SKIPBLK:
                tape_next_block(i);
                emulated_tapes[i].executing_command = RESET;
                tape->status                        = DEVICE_READY;
                break;
            case READBLK:
                if (read_tape_block(i, (unsigned char *)(uint64_t)tape->data0) < 0) {
                    tape->status = READ_ERROR;
                } else {
                    tape->status = DEVICE_READY;
                    if (!(interrupt_mask & (1 << IL_TAPE))) {
                        intline                  = interrupt_lines[IL_TAPE] | ((uint8_t)(1 << i));
                        interrupt_lines[IL_TAPE] = intline;
                    }
                }
                emulated_tapes[i].executing_command = RESET;
                break;

            case WRITEBLK:
                if (write_tape_block(i, (unsigned char *)(uint64_t)tape->data0) < 0) {
                    tape->status = READ_ERROR;
                } else {
                    tape->status = DEVICE_READY;
                    if (!(interrupt_mask & (1 << IL_TAPE))) {
                        intline                  = interrupt_lines[IL_TAPE] | ((uint8_t)(1 << i));
                        interrupt_lines[IL_TAPE] = intline;
                    }
                }
                emulated_tapes[i].executing_command = RESET;
                break;
            default:
                break;
        }
    } else {
        switch (tape->command) {
            case RESET:
                tape->status = DEVICE_READY;
                break;
            case ACK:
                emulated_tapes[i].old_command = ACK;
                tape->status                  = DEVICE_READY;
                intline                       = interrupt_lines[IL_TAPE] & ((uint8_t)(~(1 << i)));
                interrupt_lines[IL_TAPE]      = intline;
                tape->command                 = RESET;
                break;
            case SKIPBLK:
                if (tape->status == DEVICE_READY) {
                    if (tape->command == emulated_tapes[i].old_command)
                        break;

                    emulated_tapes[i].old_command       = SKIPBLK;
                    emulated_tapes[i].executing_command = SKIPBLK;
                    tape->status                        = DEVICE_BUSY;
                }
            case READBLK:
                if (tape->status == DEVICE_READY) {
                    if (tape->command == emulated_tapes[i].old_command)
                        break;
                    emulated_tapes[i].old_command       = READBLK;
                    emulated_tapes[i].executing_command = READBLK;
                    tape->status                        = DEVICE_BUSY;
                }
                break;
            case WRITEBLK:
                if (tape->status == DEVICE_READY) {
                    if (tape->command == emulated_tapes[i].old_command)
                        break;
                    emulated_tapes[i].old_command       = WRITEBLK;
                    emulated_tapes[i].executing_command = WRITEBLK;
                    tape->status                        = DEVICE_BUSY;
                }
                break;
        }
    }
}