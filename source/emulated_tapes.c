#include "uart.h"
#include "emulated_tapes.h"
#include "sd.h"
#include "fat.h"

unsigned int tape_clusters[MAX_TAPES];
unsigned int tape_block_index[MAX_TAPES];

void init_emulated_tapes() {
    int        i;
    char       nome[] = "TAPEn      ";
    tapereg_t *tape;

    for (i = 0; i < MAX_TAPES; i++) {
        tape                = (tapereg_t *)DEV_REG_ADDR(IL_TAPE, i);
        nome[4]             = '0' + i;
        tape_clusters[i]    = fat_getcluster(nome);
        tape_block_index[i] = 0;

        if (tape_clusters[i]) {
            tape->status = DEVICE_READY;
            uart0_puts("Found tape device ");
            uart0_puts(nome);
            uart0_puts("\n");
        } else {
            tape->status = DEVICE_NOT_INSTALLED;
        }
        tape->command = RESET;
    }
}

unsigned int read_tape_block(int tape, unsigned char *buffer) {
    if (tape_clusters[tape] == 0)
        return 0;
    return fat_transferfile(tape_clusters[tape], buffer, tape_block_index[tape], SD_READBLOCK);
}

unsigned int write_tape_block(int tape, unsigned char *buffer) {
    if (tape_clusters[tape] == 0)
        return 0;
    return fat_transferfile(tape_clusters[tape], buffer, tape_block_index[tape], SD_WRITEBLOCK);
}

void tape_next_block(int tape) { tape_block_index[tape]++; }

void tape_prev_block(int tape) {
    if (tape_block_index[tape > 0])
        tape_block_index[tape]--;
}