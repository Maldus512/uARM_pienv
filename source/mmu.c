#include <stdint.h>
#include "arch.h"
#include "mmu.h"
#include "uart.h"

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
        uint64_t    MemAttr : 3;       // @2-5
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


/* Level0 1:1 mapping to Level1 */
static __attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR Level0map1to1[512] = {0};

/* The Level 3 ... 1 to 1 mapping */
/* This will have 1024 entries x 2M so a full range of 2GB */
static __attribute__((aligned(4096))) VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR Level1map1to1[1024] = {0};


void init_page_table(void) {
    uint32_t base = 0;

    // initialize 1:1 mapping for TTBR0
    // 1024MB of ram memory (some belongs to the VC)
    // default: 880 MB ARM ram, 128MB VC

    /* 880Mb of ram */
    for (base = 0; base < 440; base++) {
        // Each block descriptor (2 MB)
        Level1map1to1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address   = (uintptr_t)base << (21 - 12),
                                                                  .AF        = 1,
                                                                  .AP        = APBITS_NO_EL0,
                                                                  .SH        = SH_INNER_SHAREABLE,
                                                                  .MemAttr   = MT_NORMAL_INDEX,
                                                                  .EntryType = 1};
    }

    /* VC ram up to 0x3F000000 */
    for (; base < 512 - 8; base++) {
        // Each block descriptor (2 MB)
        Level1map1to1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)base << (21 - 12),
                                                                  .AF      = 1,
                                                                  //.AP        = APBITS_NO_LIMIT,
                                                                  .MemAttr   = MT_NORMAL_NC_INDEX,
                                                                  .EntryType = 1};
    }

    /* 16 MB peripherals at 0x3F000000 - 0x40000000*/
    for (; base < 512; base++) {
        // Each block descriptor (2 MB)
        Level1map1to1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)base << (21 - 12),
                                                                  .AF      = 1,
                                                                  //.AP        = APBITS_NO_LIMIT,
                                                                  .MemAttr   = MT_DEVICE_NGNRNE_INDEX,
                                                                  .EntryType = 1};
    }

    // 2 MB for mailboxes at 0x40000000
    // shared device, never execute
    Level1map1to1[512] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)512 << (21 - 12),
                                                             .AF      = 1,
                                                             //.AP        = APBITS_NO_LIMIT,
                                                             .MemAttr   = MT_DEVICE_NGNRNE_INDEX,
                                                             .EntryType = 1};

    // unused up to 0x7FFFFFFF
    for (base = 513; base < 1024; base++) {
        Level1map1to1[base].Raw64 = 0;
    }

    // All the rest of L2 entries are empty
    for (uint_fast16_t i = 0; i < 512; i++) {
        Level0map1to1[i].Raw64 = 0;
    }

    // Just 2 valid entries mapping the 2GB in stage2
    Level0map1to1[0] =
        (VMSAv8_64_NEXTLEVEL_DESCRIPTOR){.NSTable = 1, .Address = (uintptr_t)&Level1map1to1[0] >> 12, .EntryType = 3};
    Level0map1to1[1] =
        (VMSAv8_64_NEXTLEVEL_DESCRIPTOR){.NSTable = 1, .Address = (uintptr_t)&Level1map1to1[512] >> 12, .EntryType = 3};

}


/**
 * Use the previous setup page translation tables
 */
void mmu_init(void) {
    uint64_t r;

    /* Set the memattrs values into mair_el1*/
    asm volatile("dsb sy");
    r = ((MT_DEVICE_NGNRNE << (MT_DEVICE_NGNRNE_INDEX * 8)) | (MT_DEVICE_NGNRE << (MT_DEVICE_NGNRE_INDEX * 8)) |
         (MT_DEVICE_GRE << (MT_DEVICE_GRE_INDEX * 8)) | (MT_NORMAL_NC << (MT_NORMAL_NC_INDEX * 8)) |
         (MT_NORMAL << (MT_NORMAL_INDEX * 8)));
    asm volatile("msr mair_el1, %0" : : "r"(r));

    // Bring both tables online and execute memory barrier
    asm volatile("msr ttbr0_el1, %0" : : "r"((uintptr_t)&Level0map1to1[0]));
    asm volatile("msr ttbr1_el1, %0" : : "r"((uintptr_t)&Level0map1to1[0]));
    asm volatile("isb");

    /* Specify mapping characteristics in translate control register
        page D10-2685 of the ARM ARM */
    r = (0b00LL << 37) |      // TBI=0, no tagging
        (0b000LL << 32) |     // IPS= 32 bit ... 000 = 32bit, 001 = 36bit, 010 = 40bit
        (0b10LL << 30) |      // TG1=4k ... options are 10=4KB, 01=16KB, 11=64KB ... take care differs from TG0
        (0b11LL << 28) |      // SH1=3 inner ... options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 =
                              // Inner Shareable
        (0b01LL << 26) |      // ORGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write
                              // thru cacheable, 11 = Write Back Non-cacheable
        (0b01LL << 24) |      // IRGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write
                              // thru cacheable, 11 = Write Back Non-cacheable
        (0b0LL << 23) |       // EPD1 ... Translation table walk disable for translations using TTBR1_EL1  0 = walk, 1 =
                              // generate fault
        (25LL << 0) |        // T1SZ=25 (512G) ... The region size is 2 POWER (64-T1SZ) bytes
        (0b00LL << 14) |      // TG0=4k  ... options are 00=4KB, 01=64KB, 10=16KB,  ... take care differs from TG1
        (0b11LL << 12) |      // SH0=3 inner ... .. options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 =
                              // Inner Shareable
        (0b01LL << 10) |      // ORGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write
                              // thru cacheable, 11 = Write Back Non-cacheable
        (0b01LL << 8) |       // IRGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write
                              // thru cacheable, 11 = Write Back Non-cacheable
        (0b0LL << 7) |     // EPD0  ... Translation table walk disable for translations using TTBR0_EL1  0 = walk, 1 =
                           // generate fault
        (25LL << 0);       // T0SZ=25 (512G)  ... The region size is 2 POWER (64-T0SZ) bytes
    asm volatile("msr tcr_el1, %0; isb" : : "r"(r));

    /* finally, toggle some bits in system control register to enable page translation
        page D10-2654 of the ARM ARM */
    asm volatile("isb; mrs %0, sctlr_el1" : "=r"(r));

    r |= 0xC00800;         // set mandatory reserved bits
    r &= ~((1 << 25) |     // clear EE, little endian translation tables
           (1 << 24) |     // clear E0E
           (1 << 23) |     // Clear SPAN (no Privileged Access Never on exceptions)
           (1 << 19) |     // clear WXN
           (1 << 12) |     // clear I, no instruction cache TODO: remove
           (1 << 4) |      // clear SA0
           (1 << 3) |      // clear SA
           (1 << 2) |      // clear C, no cache at all TODO: Remove
           (1 << 1));      // clear A, no aligment check
    r |= (1 << 0);         // set M, enable MMU

    /*r |= 0xC00800;       // set mandatory reserved bits
    r |= (1 << 12) |     // I, Instruction cache enable. This is an enable bit for instruction caches at EL0 and EL1
         (1 << 4) |      // SA0, tack Alignment Check Enable for EL0
         (1 << 3) |      // SA, Stack Alignment Check Enable
         (1 << 2) |      // C, Data cache enable. This is an enable bit for data caches at EL0 and EL1
         (1 << 1) |      // A, Alignment check enable bit
         (1 << 0);       // set M, enable MMU*/
    asm volatile("msr sctlr_el1, %0; isb" : : "r"(r));
}