#ifndef __SD_H__
#define __SD_H__

#include <stdint.h>

#define EMMC_BASE (IO_BASE + 0x00300000)

#define SD_OK 0
#define SD_TIMEOUT -1
#define SD_ERROR -2

struct EMMCREG {
    volatile uint32_t ARG2;
    volatile uint32_t BLKSIZECNT;
    volatile uint32_t ARG1;
    volatile uint32_t CMDTM;
    volatile uint32_t RESP0;
    volatile uint32_t RESP1;
    volatile uint32_t RESP2;
    volatile uint32_t RESP3;
    volatile uint32_t DATA;
    volatile uint32_t STATUS;
    volatile uint32_t CONTROL0;
    volatile uint32_t CONTROL1;
    volatile uint32_t INTERRUPT;
    volatile uint32_t IRPT_MASK;
    volatile uint32_t IRPT_EN;
    volatile uint32_t CONTROL2;
    volatile uint32_t FORCE_IRPT;
    volatile uint32_t BOOT_TIMEOUT;
    volatile uint32_t DBG_SEL;
    volatile uint32_t EXRDFIFO_CFG;
    volatile uint32_t TUNE_STEP;
    volatile uint32_t TUNE_STEP_STD;
    volatile uint32_t TUNE_STEP_DDR;
    volatile uint32_t SPI_INT_SPT;
    volatile uint32_t SLOTISR_VER;
};

typedef enum { SD_READBLOCK = 0, SD_WRITEBLOCK } readwrite_t;

#define EMMC ((struct EMMCREG *)EMMC_BASE)

int sd_init();
int sd_transferblock(unsigned int lba, unsigned char *buffer, unsigned int num, readwrite_t readwrite);

#endif