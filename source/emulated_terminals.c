#include <stdint.h>
#include "arch.h"
#include "emulated_terminals.h"
#include "lfb.h"

#define WIDTH (REQUESTED_WIDTH / 2)
#define HEIGHT (REQUESTED_HEIGHT / 2)

static int terminal_coordinates[MAX_TERMINALS][2];


void init_emulated_terminals() {
    int        i;
    termreg_t *terminal;
    uint8_t *  device_installed = (uint8_t *)DEVICE_INSTALLED;
    uint8_t    tmp;

    device_installed[IL_TERMINAL] = 0x00;
    for (i = 0; i < MAX_TERMINALS; i++) {
        terminal                      = (termreg_t *)DEV_REG_ADDR(IL_TERMINAL, i);
        terminal->recv_status         = DEVICE_READY;
        terminal->transm_status       = DEVICE_READY;
        terminal->recv_command        = RESET;
        terminal->transm_command      = RESET;
        tmp                           = device_installed[IL_TERMINAL] | ((uint8_t)(1 << i));
        device_installed[IL_TERMINAL] = tmp;
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


void manage_emulated_terminal(int i) {
    uint8_t *  interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t    interrupt_mask  = *((uint8_t *)INTERRUPT_MASK);
    termreg_t *terminal        = (termreg_t *)DEV_REG_ADDR(IL_TERMINAL, i);

    switch (terminal->transm_command & 0xFF) {
        case RESET:
            terminal->transm_command = RESET;
            terminal->transm_status  = DEVICE_READY;
            /* Reset command also removes interrupt */
            interrupt_lines[IL_TERMINAL] &= ~(1 << i);
            break;
        case ACK:
            terminal->transm_command     = RESET;
            terminal->transm_status      = DEVICE_READY;
            interrupt_lines[IL_TERMINAL] = 0;     //&= ~(1 << i);
            break;
        case TRANSMIT_CHAR:
            if ((terminal->transm_status & 0xFF) == DEVICE_READY) {
                terminal->transm_status = DEVICE_BUSY;
                // TODO: there might be sync problems between processes due to this system
                // I need to check if it's meant to be like that or I should avoid them
            } else if ((terminal->transm_status & 0xFF) == DEVICE_BUSY) {
                terminal_send(i, terminal->transm_command >> 8);
                terminal->transm_status = CHAR_TRANSMIT;
                if (!(interrupt_mask & (1 << IL_TERMINAL))) {
                    interrupt_lines[IL_TERMINAL] |= 1 << i;
                }
            }
            break;
    }
}