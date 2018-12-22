#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>     // Needed for uint8_t, uint32_t, etc

#define MT_DEVICE_NGNRNE_INDEX 0
#define MT_DEVICE_NGNRE_INDEX 1
#define MT_DEVICE_GRE_INDEX 2
#define MT_NORMAL_NC_INDEX 3
#define MT_NORMAL_INDEX 4

#define MT_DEVICE_NGNRNE 0x00ul
#define MT_DEVICE_NGNRE 0x04ul
#define MT_DEVICE_GRE 0x0Cul
#define MT_NORMAL_NC 0x44ul
#define MT_NORMAL 0xFFul

extern uint32_t table_loaded;

extern uint32_t table_loaded;

void init_page_table(void);

void     initMMU(void);
uint64_t virtualmap(uint32_t phys_addr, uint8_t memattrs);

#endif