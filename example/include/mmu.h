#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>     // Needed for uint8_t, uint32_t, etc


/* REFERENCE FOR STAGE 1 NEXT LEVEL ENTRY */
/* ARMv8-A_Architecture_Reference_Manual_(Issue_A.a).pdf  D5-1776 */
typedef enum {
    APTABLE_NOEFFECT = 0,     // No effect
    APTABLE_NO_EL0   = 1,     // Access at EL0 not permitted, regardless of permissions in subsequent levels of lookup
    APTABLE_NO_WRITE = 2,     // Write access not permitted, at any Exception level, regardless of permissions in
                              // subsequent levels of lookup
    APTABLE_NO_WRITE_EL0_READ =
        3     // Write access not permitted,at any Exception level, Read access not permitted at EL0.
} APTABLE_TYPE;

typedef enum {
    APBITS_NO_EL0              = 0,
    APBITS_NO_LIMIT            = 1,
    APBITS_NO_WRITE_ELn_NO_EL0 = 2,
    APBITS_NO_WRITE_ELn        = 3,
} APBITS_TYPE;

/* Format of table descriptor depicted from page D4-2143 of the ARM ARM */
typedef union {
    struct __attribute__((__packed__)) {
        uint64_t     EntryType : 2;           // @0-1		Always 3 for a page table
        uint64_t     _reserved2_11 : 10;      // @2-11	Set to 0
        uint64_t     Address : 36;            // @12-47	36 Bits of address
        uint64_t     _reserved48_58 : 11;     // @48-58	Set to 0
        uint64_t     PXNTable : 1;            // @59 Privileged eXecute Never limit for subsequent levels of lookup
        uint64_t     XNTable : 1;             // @60 eXecute Never limit for subsequent levels of lookup
        APTABLE_TYPE APTable : 2;             // @61-62	Access permission limit for subsequent levels of lookup
        uint64_t     NSTable : 1;             // @63 Secure state, for accesses from Non-secure state this bit is RES0
                                              // and is ignored
    };
    uint64_t Raw64;     // Raw 64bit access
} VMSAv8_64_NEXTLEVEL_DESCRIPTOR;


typedef union {
    /* Memory configuration is described from page D4-2149 of the ARM ARM */
    struct __attribute__((__packed__)) {
        uint64_t    EntryType : 2;     // @0-1		Always 1 for a block table
        uint64_t    MemAttr : 3;       // @2-4
        uint64_t    NS : 1;
        APBITS_TYPE AP : 2;     // @6-7     Data Access permission bits
        enum {
            SH_OUTER_SHAREABLE = 2,     //			Outter shareable
            SH_INNER_SHAREABLE = 3,     //			Inner shareable
        } SH : 2;                              // @8-9 Shareability field
        uint64_t AF : 1;                 // @10 Access flag; if 0 an access to this page leads to an TLB Access fault
        uint64_t nG : 1;                 // @11 not Global bit (ASID management for TLB)
        uint64_t Address : 36;           // @12-47 36 Bits of address
        uint64_t _reserved48_50 : 3;     // @48-51 Set to 0
        uint64_t DBM : 1;                // @51 Dirty Bit Modifier
        uint64_t Contiguous : 1;         // @52 Contiguous bit (for TLB optimization)
        uint64_t PXN : 1;                // @53 Privileged eXecute Never bit; execution is not allowed at EL0 if 1
        uint64_t XN : 1;                 // @54 eXecute Never bit; execution is not allowed if 1
        uint64_t _reserved55_63 : 9;     // @55-63 Set to 0
    };
    uint64_t Raw64;     // Raw 64bit access
} VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR;


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

void init_page_tables(VMSAv8_64_NEXTLEVEL_DESCRIPTOR *level0, VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR *level1,
                      APBITS_TYPE permission);

void     mmu_init(void);

#endif