#include <stdint.h>
#include "arch.h"
#include "emulated_terminals.h"
#include "lfb.h"
#include "timers.h"
#include "utils.h"

#define WIDTH (REQUESTED_WIDTH / 2)
#define HEIGHT (REQUESTED_HEIGHT / 2)

static int terminal_coordinates[MAX_TERMINALS][2];

static printer_internal_state_t printers[MAX_PRINTERS] = {0};


void init_emulated_terminals() {
    int      i;
    uint8_t *device_installed = (uint8_t *)DEVICE_INSTALLED;

    device_installed[IL_TERMINAL] = 0x00;

    for (i = 0; i < MAX_PRINTERS; i++) {
        printers[i].internal_registers.command = RESET;
        printers[i].internal_registers.status  = DEVICE_READY;
    }
}

const int terminal_starting_coordinates[MAX_TERMINALS][2] = {
    {0, 0},
    {WIDTH / 9 + 1, 0},
    {0, HEIGHT / 16 + 1},
    {WIDTH / 9 + 1, HEIGHT / 16 + 1},
};

void terminal_send(int num, char c) {
    int width, height, pitch;
    int x      = terminal_coordinates[num][0] + terminal_starting_coordinates[num][0];
    int y      = terminal_coordinates[num][1] + terminal_starting_coordinates[num][1];
    int limitx = terminal_starting_coordinates[num][0] + WIDTH / 9;
    int limity = terminal_starting_coordinates[num][1] + HEIGHT / 16 + (num > 1 ? -1 : 0);
    int i;
    screen_resolution(&width, &height, &pitch);

    if (c == '\r') {
        x = terminal_starting_coordinates[num][0];
    } else if (x + 1 >= limitx || c == '\n') {
        if (c == '\n') {
            for (i = x; i < limitx; i++) {
                lfb_send(i, y, ' ');
            }
        }
        x = terminal_starting_coordinates[num][0];
        if (!(y + 1 >= limity)) {
            y++;
        } else {
            y = terminal_starting_coordinates[num][1];
        }
        if (x + 1 >= limitx && c != '\n') {
            lfb_send(x, y, c);
            x++;
        }
    } else {
        lfb_send(x, y, c);
        x++;
    }
    terminal_coordinates[num][0] = x - terminal_starting_coordinates[num][0];
    terminal_coordinates[num][1] = y - terminal_starting_coordinates[num][1];
}

void emulated_printer_mailbox(int i, printreg_t *registers) {
    uint8_t *interrupt_lines = (uint8_t *)INTERRUPT_LINES;

    if (registers == NULL)
        return;

    if (printers[i].internal_registers.status == DEVICE_NOT_INSTALLED && registers->command != READ_REGISTERS)
        return;

    printers[i].internal_registers.mailbox = 1;

    switch (registers->command & 0xFF) {
        case RESET:
            printers[i].internal_registers.status  = DEVICE_READY;
            printers[i].internal_registers.command = RESET;
            /* Reset command also removes interrupt */
            interrupt_lines[IL_PRINTER] &= ~(1 << i);
            memcpy(registers, &printers[i].internal_registers, sizeof(printreg_t));
            break;
        case ACK:
            if (printers[i].internal_registers.status != DEVICE_BUSY) {
                printers[i].internal_registers.command = ACK;
                printers[i].internal_registers.status  = DEVICE_READY;
                interrupt_lines[IL_PRINTER] &= ~(1 << i);
                memcpy(registers, &printers[i].internal_registers, sizeof(printreg_t));
            } else {
                registers->mailbox = 2;
            }
            break;
        case READ_REGISTERS:
            memcpy(registers, &printers[i].internal_registers, sizeof(printreg_t));
            break;
        case PRINT_CHAR:
            if (printers[i].internal_registers.status != DEVICE_BUSY) {
                printers[i].internal_registers.status  = DEVICE_BUSY;
                printers[i].internal_registers.command = PRINT_CHAR;
                printers[i].internal_registers.data0   = registers->data0;
                printers[i].mailbox_registers          = registers;
                memcpy(registers, &printers[i].internal_registers, sizeof(printreg_t));
                set_device_timer(100);
            } else {
                registers->mailbox = 2;
            }
            break;
        default:
            printers[i].internal_registers.status = ILLEGAL_OPERATION;
            memcpy(registers, &printers[i].internal_registers, sizeof(printreg_t));
            break;
    }
}

void manage_emulated_printer(int i) {
    uint8_t *interrupt_lines = (uint8_t *)INTERRUPT_LINES;

    if (printers[i].internal_registers.status == DEVICE_BUSY) {
        switch (printers[i].internal_registers.command) {
            case PRINT_CHAR:
                terminal_send(i, printers[i].internal_registers.data0);
                printers[i].internal_registers.status = DEVICE_READY;
                interrupt_lines[IL_TERMINAL] |= 1 << i;
                printers[i].executing_command = RESET;
                memcpy(printers[i].mailbox_registers, &printers[i].internal_registers, sizeof(printreg_t));
                printers[i].mailbox_registers = NULL;
                break;
            default:
                break;
        }
    }
}