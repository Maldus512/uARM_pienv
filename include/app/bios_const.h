#ifndef __BIOS_CONST_H__
#define __BIOS_CONST_H__

#define BIOS_SRV_SYS 8
#define BIOS_SRV_BP 9
#define BIOS_SRV_HALT 1
#define BIOS_SRV_PANIC 2
#define BIOS_SRV_LDST 3
#define BIOS_SRV_WAIT 4

#define BIOS_INT_SYS 10

#define INTERRUPT_OLDAREA 0x70000
#define SYNCHRONOUS_OLDAREA 0x70200
#define SYNCHRONOUS_OFFSET 0x200

#define INTERRUPT_HANDLER 0x70400
#define SYNCHRONOUS_HANDLER 0x70600

#define spin_cpu0 0xd8
#define spin_cpu1 0xe0
#define spin_cpu2 0xe8
#define spin_cpu3 0xf0
#endif