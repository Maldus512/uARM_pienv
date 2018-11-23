#include <stdint.h>
#include "hardwareprofile.h"
#include "mmu.h"

#define PAGESIZE    4096

// granularity
#define PT_PAGE     0b11        // 4k granule
#define PT_BLOCK    0b01        // 2M granule
// accessibility
#define PT_KERNEL   (0<<6)      // privileged, supervisor EL1 access only
#define PT_USER     (1<<6)      // unprivileged, EL0 access allowed
#define PT_RW       (0<<7)      // read-write
#define PT_RO       (1<<7)      // read-only
#define PT_AF       (1<<10)     // accessed flag
#define PT_NX       (1UL<<54)   // no execute
// shareability
#define PT_OSH      (2<<8)      // outter shareable
#define PT_ISH      (3<<8)      // inner shareable
// defined in MAIR register
#define PT_MEM      (0<<2)      // normal memory
#define PT_DEV      (1<<2)      // device MMIO
#define PT_NC       (2<<2)      // non-cachable

#define TTBR_ENABLE 1



// get addresses from linker
extern volatile unsigned char _data;
extern volatile unsigned char _page_table;


//TODO: Without this unused array it doesn't work, there must be some error in the attribute setting
static __attribute__((aligned(4096), unused)) uint64_t paging2[515*512] = {0};

void initMMU() {
    unsigned long data_page = (unsigned long)&_data/PAGESIZE;
    unsigned long r, b, *paging=(unsigned long*)&_page_table;
    uint64_t table;
    uint64_t address;

    /* create MMU translation tables at _page_table */

    // TTBR0, identity L1
    paging[0]=(unsigned long)((unsigned char*)&paging[1*512]) |    // physical address
        PT_PAGE |     // it has the "Present" flag, which must be set, and we have area in it mapped by pages
        PT_AF |       // accessed flag. Without this we're going to have a Data Abort exception
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    paging[1]=(unsigned long)((unsigned char*)&paging[2*512]) |    // physical address
        PT_PAGE |     // it has the "Present" flag, which must be set, and we have area in it mapped by pages
        PT_AF |       // accessed flag. Without this we're going to have a Data Abort exception
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

     // identity L2 2M blocks
    b=MMIO_BASE>>21;
    // skip 0th, as we're about to map it by L3
    for(r=0;r<512;r++) {
        paging[1*512+r]=(unsigned long)((unsigned char*)&paging[(3+r)*512]) |  // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_USER |     // non-privileged
        (r>=b? PT_OSH|PT_DEV : PT_ISH|PT_MEM); // different attributes for device memory
    }

//(VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .AP = 0, .Address = (uintptr_t)512 << (21 - 12), .AF = 1, .MemAttr = MT_DEVICE_NGNRNE, .EntryType = 1 };
    // Map the last device part as 2M because nobody cares
    paging[2*512]=(unsigned long)((512<<21)) |  // physical address
        PT_BLOCK |    // map 2M block
        PT_AF |       // accessed flag
        PT_NX |       // no execute
        PT_USER |     // non-privileged
        PT_OSH|PT_DEV; // different attributes for device memory

    // identity L3
    for (table = 0 ; table < 512; table++) {
        for(r=0;r<512;r++) {
            address = (table*512+r);
            paging[(3+table)*512+r]=(unsigned long)(address<<12) |   // physical address
            PT_PAGE |     // map 4k
            PT_AF |       // accessed flag
            PT_USER |     // non-privileged
            PT_ISH |      // inner shareable
            ((address<0x80||address>data_page)? PT_RW|PT_NX : PT_RO); // different for code and data
        }
    }


    // check for 4k granule and at least 36 bits physical address bus */
    asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));
    b=r&0xF;
    if(r&(0xF<<28)/*4k*/ || b<1/*36 bits*/) {
        uart0_puts("ERROR: 4k granule or 36 bit address space not supported\n");
        return;
    }

    // first, set Memory Attributes array, indexed by PT_MEM, PT_DEV, PT_NC in our example
    r=  (0xFF << 0) |    // AttrIdx=0: normal, IWBWA, OWBWA, NTR
        (0x04 << 8) |    // AttrIdx=1: device, nGnRE (must be OSH too)
        (0x44 <<16);     // AttrIdx=2: non cacheable
    asm volatile ("msr mair_el1, %0" : : "r" (r));

    // next, specify mapping characteristics in translate control register
    r=  (0b00LL << 37) | // TBI=0, no tagging
        (b << 32) |      // IPS=autodetected
        (0b10LL << 30) | // TG1=4k
        (0b11LL << 28) | // SH1=3 inner
        (0b01LL << 26) | // ORGN1=1 write back
        (0b01LL << 24) | // IRGN1=1 write back
        (0b0LL  << 23) | // EPD1 enable higher half
        (25LL   << 16) | // T1SZ=25, 3 levels (512G)
        (0b00LL << 14) | // TG0=4k
        (0b11LL << 12) | // SH0=3 inner
        (0b01LL << 10) | // ORGN0=1 write back
        (0b01LL << 8) |  // IRGN0=1 write back
        (0b0LL  << 7) |  // EPD0 enable lower half
        (25LL   << 0);   // T0SZ=25, 3 levels (512G)
    asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));

    // tell the MMU where our translation tables are. TTBR_ENABLE bit not documented, but required
    // lower half, user space
    asm volatile ("msr ttbr0_el1, %0" : : "r" ((unsigned long)&_page_table + TTBR_ENABLE));
    //asm volatile ("msr ttbr0_el1, %0" : : "r" ((uintptr_t)&Stage1map1to1[0] + TTBR_ENABLE));
	asm volatile("isb");

    // upper half, kernel space
    //asm volatile ("msr ttbr1_el1, %0" : : "r" ((unsigned long)&_page_table + TTBR_ENABLE + PAGESIZE));

    // finally, toggle some bits in system control register to enable page translation
    asm volatile ("dsb ish; isb; mrs %0, sctlr_el1" : "=r" (r));
    r|=0xC00800;     // set mandatory reserved bits
    r&=~((1<<25) |   // clear EE, little endian translation tables
         (1<<24) |   // clear E0E
         (1<<19) |   // clear WXN
         (1<<12) |   // clear I, no instruction cache
         (1<<4) |    // clear SA0
         (1<<3) |    // clear SA
         (1<<2) |    // clear C, no cache at all
         (1<<1));    // clear A, no aligment check
    r|=  (1<<0);     // set M, enable MMU
    asm volatile ("msr sctlr_el1, %0; isb" : : "r" (r));
}
