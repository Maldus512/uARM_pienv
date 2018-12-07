#include <stdint.h>
#include "emulated_devices.h"
#include "lfb.h"


volatile termreg_t terminals[MAX_TERMINALS];

static int terminal_coordinates[MAX_TERMINALS][2];

void terminal_send(int num, char c) {

    if (c == '\r') {
        terminal_coordinates[num][0] = 0;
    }
    else if (c == '\n') {
        terminal_coordinates[num][0] = 0;
        terminal_coordinates[num][1]++;
    }
    else {
        lfb_send(terminal_coordinates[num][0], terminal_coordinates[num][1], c);
        terminal_coordinates[num][0]++;
    }
}

void *DEV_REG_ADDR(int line, int dev) {
    return (void*) &terminals[line];
}