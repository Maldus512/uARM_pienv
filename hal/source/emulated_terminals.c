#include <stdint.h>
#include "emulated_terminals.h"
#include "lfb.h"

#define WIDTH (REQUESTED_WIDTH / 2)
#define HEIGHT (REQUESTED_HEIGHT / 2)

volatile termreg_t terminals[MAX_TERMINALS];

static int terminal_coordinates[MAX_TERMINALS][2];

const int terminal_starting_coordinates[MAX_TERMINALS][2] = {
    {0, 0},
    {WIDTH / 9 + 1, 0},
    {0, HEIGHT / 16 + 1},
    {WIDTH / 9 + 1, HEIGHT / 16 + 1},
};

void terminal_send(int num, char c) {
    int  width, height, pitch;
    int  x          = terminal_coordinates[num][0] + terminal_starting_coordinates[num][0];
    int  y          = terminal_coordinates[num][1] + terminal_starting_coordinates[num][1];
    int  limitx     = terminal_starting_coordinates[num][0] + WIDTH / 9;
    int  limity     = terminal_starting_coordinates[num][1] + HEIGHT / 16 + (num > 1 ? -1 : 0);
    char f_nextline = 0;
    screen_resolution(&width, &height, &pitch);

    if (c == '\r') {
        x = terminal_starting_coordinates[num][0];
    } else if (x + 1 >= limitx || c == '\n') {
        f_nextline = (x + 1 >= limitx);
        x          = terminal_starting_coordinates[num][0];
        if (!(y + 1 >= limity)) {
            y++;
        } else {
            y = terminal_starting_coordinates[num][1];
        }
        if (f_nextline) {
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

void *DEV_REG_ADDR(int line, int dev) { return (void *)&terminals[dev]; }