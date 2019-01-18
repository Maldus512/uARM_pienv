#ifndef __BIOS_CONST_H__
#define __BIOS_CONST_H__

#define BIOS_SRV_SYS 8
#define BIOS_SRV_BP 9
#define BIOS_SRV_HALT 1
#define BIOS_SRV_PANIC 2
#define BIOS_SRV_LDST 3
#define BIOS_SRV_WAIT 4

#define BIOS_INT_SYS 10

#define EXCEPTION_OLDAREA       0x70000
#define SYNCHRONOUS_OFFSET      0x00800
#define ABORT_OFFSET            0x01000

#define CORE_OFFSET               0x200
#define INTERRUPT_OLDAREA       0x70000UL
#define SYNCHRONOUS_OLDAREA     0x70800UL
#define ABORT_OLDAREA           0x71000UL


#define INTERRUPT_HANDLER 0x72000
#define SYNCHRONOUS_HANDLER 0x72200
#define ABORT_HANDLER 0x72400



#define KERNEL_CORE0_SP     0x72C00UL
#define KERNEL_CORE1_SP     0x72C08UL
#define KERNEL_CORE2_SP     0x72C10UL
#define KERNEL_CORE3_SP     0x72C18UL

#define spin_cpu0 0xd8
#define spin_cpu1 0xe0
#define spin_cpu2 0xe8
#define spin_cpu3 0xf0
#endif