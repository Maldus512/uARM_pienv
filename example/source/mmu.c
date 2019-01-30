#include <stdint.h>
#include "arch.h"
#include "mmu.h"

__attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR Level0map1to1_el0[512] = {0};
__attribute__((aligned(4096))) VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR Level1map1to1_el0[1024] = {0};


void init_page_tables(VMSAv8_64_NEXTLEVEL_DESCRIPTOR *level0, VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR *level1,
                      APBITS_TYPE permission) {
    uint32_t base = 0;

    // initialize 1:1 mapping for TTBR0
    // 1024MB of ram memory (some belongs to the VC)
    // default: 880 MB ARM ram, 128MB VC

    /* 880Mb of ram */
    for (base = 0; base < 440; base++) {
        // Each block descriptor (2 MB)
        level1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address   = (uintptr_t)base << (21 - 12),
                                                           .AF        = 1,
                                                           .AP        = permission,
                                                           .SH        = SH_INNER_SHAREABLE,
                                                           .MemAttr   = MT_NORMAL_INDEX,
                                                           .EntryType = 1};
    }

    /* VC ram up to 0x3F000000 */
    for (; base < 512 - 8; base++) {
        // Each block descriptor (2 MB)
        level1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)base << (21 - 12),
                                                           .AF      = 1,
                                                           .AP        = permission,
                                                           //.AP        = APBITS_NO_LIMIT,
                                                           .MemAttr   = MT_NORMAL_NC_INDEX,
                                                           .EntryType = 1};
    }

    /* 16 MB peripherals at 0x3F000000 - 0x40000000*/
    for (; base < 512; base++) {
        // Each block descriptor (2 MB)
        level1[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)base << (21 - 12),
                                                           .AF      = 1,
                                                           .AP        = APBITS_NO_LIMIT,
                                                           .MemAttr   = MT_DEVICE_NGNRNE_INDEX,
                                                           .EntryType = 1};
    }

    // 2 MB for mailboxes at 0x40000000
    // shared device, never execute
    level1[512] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)512 << (21 - 12),
                                                      .AF      = 1,
                                                      .AP        = APBITS_NO_LIMIT,
                                                      .MemAttr   = MT_DEVICE_NGNRNE_INDEX,
                                                      .EntryType = 1};

    // unused up to 0x7FFFFFFF
    for (base = 513; base < 1024; base++) {
        level1[base].Raw64 = 0;
    }

    // All the rest of L2 entries are empty
    for (uint_fast16_t i = 0; i < 512; i++) {
        level0[i].Raw64 = 0;
    }

    // Just 2 valid entries mapping the 2GB in stage2
    level0[0] =
        (VMSAv8_64_NEXTLEVEL_DESCRIPTOR){.NSTable = 1, .Address = (uintptr_t)&level1[0] >> 12, .EntryType = 3};
    level0[1] =
        (VMSAv8_64_NEXTLEVEL_DESCRIPTOR){.NSTable = 1, .Address = (uintptr_t)&level1[512] >> 12, .EntryType = 3};
}


void init_page_table_el0(void) {
    uint32_t base       = 0;
    uint32_t actualbase = 0;


    // initialize 1:1 mapping for TTBR0
    // 1024MB of ram memory (some belongs to the VC)
    // default: 880 MB ARM ram, 128MB VC

    /* 880Mb of ram */
    for (base = 0; base < 440; base++) {
        // Each block descriptor (2 MB)
        /*if ((base << 21) >= 0x1000000) {
            actualbase = base - (0x1000000UL << 21);
        } else {*/
            actualbase = base;
        //}
        Level1map1to1_el0[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address   = (uintptr_t)actualbase << (21 - 12),
                                                                      .AF        = 1,
                                                                      .AP        = APBITS_NO_LIMIT,
                                                                      .SH        = SH_INNER_SHAREABLE,
                                                                      .MemAttr   = MT_NORMAL_INDEX,
                                                                      .EntryType = 1};
    }

    /* VC ram up to 0x3F000000 */
    for (; base < 512 - 8; base++) {
        // Each block descriptor (2 MB)
        Level1map1to1_el0[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)base << (21 - 12),
                                                                      .AF      = 1,
                                                                      //.AP        = APBITS_NO_LIMIT,
                                                                      .MemAttr   = MT_NORMAL_NC_INDEX,
                                                                      .EntryType = 1};
    }

    /* 16 MB peripherals at 0x3F000000 - 0x40000000*/
    for (; base < 512; base++) {
        // Each block descriptor (2 MB)
        Level1map1to1_el0[base] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address   = (uintptr_t)base << (21 - 12),
                                                                      .AF        = 1,
                                                                      .AP        = APBITS_NO_LIMIT,
                                                                      .MemAttr   = MT_DEVICE_NGNRNE_INDEX,
                                                                      .EntryType = 1};
    }

    // 2 MB for mailboxes at 0x40000000
    // shared device, never execute
    Level1map1to1_el0[512] = (VMSAv8_64_STAGE1_BLOCK_DESCRIPTOR){.Address = (uintptr_t)512 << (21 - 12),
                                                                 .AF      = 1,
                                                                 .AP        = APBITS_NO_LIMIT,
                                                                 .MemAttr   = MT_DEVICE_NGNRNE_INDEX,
                                                                 .EntryType = 1};

    // unused up to 0x7FFFFFFF
    for (base = 513; base < 1024; base++) {
        Level1map1to1_el0[base].Raw64 = 0;
    }

    // All the rest of L2 entries are empty
    for (uint_fast16_t i = 0; i < 512; i++) {
        Level0map1to1_el0[i].Raw64 = 0;
    }

    // Just 2 valid entries mapping the 2GB in stage2
    Level0map1to1_el0[0] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR){
        .NSTable = 1, .Address = (uintptr_t)&Level1map1to1_el0[0] >> 12, .EntryType = 3};
    Level0map1to1_el0[1] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR){
        .NSTable = 1, .Address = (uintptr_t)&Level1map1to1_el0[512] >> 12, .EntryType = 3};
}

