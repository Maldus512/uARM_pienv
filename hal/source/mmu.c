#include <stdint.h>
#include "hardwareprofile.h"
#include "mmu.h"

/* REFERENCE FOR STAGE 1 NEXT LEVEL ENTRY */
/* ARMv8-A_Architecture_Reference_Manual_(Issue_A.a).pdf  D5-1776 */
typedef enum {
	APTABLE_NOEFFECT = 0,			// No effect
	APTABLE_NO_EL0 = 1,				// Access at EL0 not permitted, regardless of permissions in subsequent levels of lookup
	APTABLE_NO_WRITE = 2,			// Write access not permitted, at any Exception level, regardless of permissions in subsequent levels of lookup
	APTABLE_NO_WRITE_EL0_READ = 3	// Write access not permitted,at any Exception level, Read access not permitted at EL0.
} APTABLE_TYPE;

/* Basic next table level descriptor ... Figure 12.10  ... top descriptor */
/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/ch12s04s01.html */
typedef union {
	struct __attribute__((__packed__)) 
	{
		uint64_t EntryType : 2;				// @0-1		Always 3 for a page table
        uint64_t AttrIndx : 3;
        uint64_t NS : 1;
        uint64_t AP : 2;
        uint64_t SH : 2;
        uint64_t AF : 1;
        uint64_t nG : 1;
		uint64_t Address : 39;				// @12-47	36 Bits of address
		uint64_t _reserved51_58 : 8;		// @51-58	Set to 0
		uint64_t PXNTable : 1;				// @59      Never allow execution from a lower EL level 
		uint64_t XNTable : 1;				// @60		Never allow translation from a lower EL level
		APTABLE_TYPE APTable : 2;			// @61-62	AP Table control .. see enumerate options
		uint64_t NSTable : 1;				// @63		Secure state, for accesses from Non-secure state this bit is RES0 and is ignored
	};
	uint64_t Raw64;							// Raw 64bit access
} VMSAv8_64_NEXTLEVEL_DESCRIPTOR;


/* Basic Stage2 block descriptor ... Figure 12.10 ... 2nd descriptor */
/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/ch12s04s01.html */
/* Attributes from section => Attribute fields in stage 2 VMSAv8-64 Block and Page descriptors */
/* https://armv8-ref.codingbelief.com/en/chapter_d4/d43_3_memory_attribute_fields_in_the_vmsav8-64_translation_table_formats_descriptors.html */

typedef union {
	struct __attribute__((__packed__))
	{
		uint64_t EntryType : 2;				// @0-1		Always 1 for a block table
		uint64_t MemAttr : 4;				// @2-5
		uint64_t AP : 2;							// @6-7
		enum {
			STAGE2_SH_OUTER_SHAREABLE = 2,	//			Outter shareable
			STAGE2_SH_INNER_SHAREABLE = 3,	//			Inner shareable
		} SH : 2;							// @8-9
		uint64_t AF : 1;					// @10		Accessable flag
		uint64_t _reserved11 : 1;			// @11		Set to 0
		uint64_t Address : 36;				// @12-47	36 Bits of address
		uint64_t _reserved48_51 : 4;		// @48-51	Set to 0
		uint64_t Contiguous : 1;			// @52		Contiguous
		uint64_t _reserved53 : 1;			// @53		Set to 0
		uint64_t XN : 1;					// @54		No execute if bit set
		uint64_t _reserved55_63 : 9;		// @55-63	Set to 0
	};
	uint64_t Raw64;							// Raw 64bit access
} VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR;

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
extern volatile unsigned char _end;


/*--------------------------------------------------------------------------}
{					 CODE TYPE STRUCTURE COMPILE TIME CHECKS	            }
{--------------------------------------------------------------------------*/
/* If you have never seen compile time assertions it's worth google search */
/* on "Compile Time Assertions". It is part of the C11++ specification and */
/* all compilers that support the standard will have them (GCC, MSC inc)   */
/*-------------------------------------------------------------------------*/
#include <assert.h>								// Need for compile time static_assert

/* Check the code type structure size */
static_assert(sizeof(VMSAv8_64_NEXTLEVEL_DESCRIPTOR) == 0x08, "VMSAv8_64_NEXTLEVEL_DESCRIPTOR should be 0x08 bytes in size");
static_assert(sizeof(VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) == 0x08, "VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR should be 0x08 bytes in size");

/* Stage1 .... 1:1 mapping to Stage2 */
static __attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR Stage1map1to1[512] = { 0 };

/* The Level 3 ... 1 to 1 mapping */
/* This will have 1024 entries x 2M so a full range of 2GB */
static __attribute__((aligned(4096))) VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR Stage2map1to1[1024] = { 0 };

void init_page_table (void) {
	uint32_t base = 0;

	// initialize 1:1 mapping for TTBR0
	// 1024MB of ram memory (some belongs to the VC)
	// default: 880 MB ARM ram, 128MB VC

	/* The 21-12 entries are because that is only for 4K granual it makes it obvious to change for other granual sizes */

	/* 880Mb of ram */
	for (base = 0; base < 440; base++) {
		// Each block descriptor (2 MB)
		Stage2map1to1[base] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .AP = 0, .Address = (uintptr_t)base << (21-12), .AF = 1, .SH = STAGE2_SH_INNER_SHAREABLE, 
			                                                        .MemAttr = MT_NORMAL, .EntryType = 1 };
	}

	/* VC ram up to 0x3F000000 */
	for (; base < 512 - 8; base++) {
		// Each block descriptor (2 MB)
		Stage2map1to1[base] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .AP = 0, .Address = (uintptr_t)base << (21 - 12), .AF = 1, .MemAttr = MT_NORMAL_NC, .EntryType = 1 };
	}

	/* 16 MB peripherals at 0x3F000000 - 0x40000000*/
	for (; base < 512; base++) {
		// Each block descriptor (2 MB)
		Stage2map1to1[base] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .AP = 0, .Address = (uintptr_t)base << (21 - 12), .AF = 1, .MemAttr = MT_DEVICE_NGNRNE, .EntryType = 1 };
	}
	
	// 2 MB for mailboxes at 0x40000000
	// shared device, never execute
	Stage2map1to1[512] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .AP = 0, .Address = (uintptr_t)512 << (21 - 12), .AF = 1, .MemAttr = MT_DEVICE_NGNRNE, .EntryType = 1 };

	// unused up to 0x7FFFFFFF
	for (base = 513; base < 1024; base++) {
		Stage2map1to1[base].Raw64 = 0;
	}

	// Just 2 valid entries mapping the 2GB in stage2
	Stage1map1to1[0] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR) { .NSTable = 1, .AP=1, .Address = (uintptr_t)&Stage2map1to1[0] >> 12, .EntryType = 3 };
	Stage1map1to1[1] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR) { .NSTable = 1,  .AP=1,.Address = (uintptr_t)&Stage2map1to1[512] >> 12, .EntryType = 3 };
	
	// All the rest of L2 entries are empty 
	for (uint_fast16_t i = 2; i < 512; i++) {
		Stage1map1to1[i].Raw64 = 0;
	}
}

void initMMU() {
    unsigned long data_page = (unsigned long)&_data/PAGESIZE;
    unsigned long r, b, *paging=(unsigned long*)&_end;


    /* create MMU translation tables at _end */

    // TTBR0, identity L1
    paging[0]=(unsigned long)((unsigned char*)&_end+2*PAGESIZE) |    // physical address
        PT_PAGE |     // it has the "Present" flag, which must be set, and we have area in it mapped by pages
        PT_AF |       // accessed flag. Without this we're going to have a Data Abort exception
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // identity L2, first 2M block
    paging[2*512]=(unsigned long)((unsigned char*)&_end+3*PAGESIZE) | // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // identity L2 2M blocks
    b=MMIO_BASE>>21;
    // skip 0th, as we're about to map it by L3
    for(r=1;r<512;r++)
        paging[2*512+r]=(unsigned long)((r<<21)) |  // physical address
        PT_BLOCK |    // map 2M block
        PT_AF |       // accessed flag
        PT_NX |       // no execute
        PT_USER |     // non-privileged
        (r>=b? PT_OSH|PT_DEV : PT_ISH|PT_MEM); // different attributes for device memory

    // identity L3
    for(r=0;r<512;r++)
        paging[3*512+r]=(unsigned long)(r*PAGESIZE) |   // physical address
        PT_PAGE |     // map 4k
        PT_AF |       // accessed flag
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        ((r<0x80||r>data_page)? PT_RW|PT_NX : PT_RO); // different for code and data

    // TTBR1, kernel L1
    paging[512+511]=(unsigned long)((unsigned char*)&_end+4*PAGESIZE) | // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_KERNEL |   // privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // kernel L2
    paging[4*512+511]=(unsigned long)((unsigned char*)&_end+5*PAGESIZE) |   // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_KERNEL |   // privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // kernel L3
    paging[5*512]=(unsigned long)(MMIO_BASE+0x00201000) |   // physical address
        PT_PAGE |     // map 4k
        PT_AF |       // accessed flag
        PT_NX |       // no execute
        PT_KERNEL |   // privileged
        PT_OSH |      // outter shareable
        PT_DEV;       // device memory


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
    asm volatile ("msr ttbr0_el1, %0" : : "r" ((unsigned long)&_end + TTBR_ENABLE));
    //asm volatile ("msr ttbr0_el1, %0" : : "r" ((uintptr_t)&Stage1map1to1[0] + TTBR_ENABLE));
	asm volatile("isb");

    // upper half, kernel space
    asm volatile ("msr ttbr1_el1, %0" : : "r" ((unsigned long)&_end + TTBR_ENABLE + PAGESIZE));

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

/**
 * Use the previous setup page translation tables
 */
void initMMU2(void)
{
    uint64_t r;
   
	/* Set the memattrs values into mair_el1*/
	asm volatile("dsb sy");
	r = ((0x00ul << (MT_DEVICE_NGNRNE * 8)) | \
		 (0x04ul << (MT_DEVICE_NGNRE * 8)) | \
		 (0x0cul << (MT_DEVICE_GRE * 8)) | \
		 (0x44ul << (MT_NORMAL_NC * 8)) | \
		 (0xfful << (MT_NORMAL * 8)));
    asm volatile ("msr mair_el1, %0" : : "r" (r));

	// Bring both tables online and execute memory barrier
	asm volatile ("msr ttbr0_el1, %0" : : "r" ((uintptr_t)&Stage1map1to1[0]));
	asm volatile("isb");

    // Specify mapping characteristics in translate control register
    r = (0b00LL << 37) | // TBI=0, no tagging
        (0b000LL << 32)| // IPS= 32 bit ... 000 = 32bit, 001 = 36bit, 010 = 40bit
        (0b10LL << 30) | // TG1=4k ... options are 10=4KB, 01=16KB, 11=64KB ... take care differs from TG0
        (0b11LL << 28) | // SH1=3 inner ... options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable
        (0b01LL << 26) | // ORGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable
        (0b01LL << 24) | // IRGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable
        (0b0LL  << 23) | // EPD1 ... Translation table walk disable for translations using TTBR1_EL1  0 = walk, 1 = generate fault
        (25LL   << 16) | // T1SZ=25 (512G) ... The region size is 2 POWER (64-T1SZ) bytes
        (0b00LL << 14) | // TG0=4k  ... options are 00=4KB, 01=64KB, 10=16KB,  ... take care differs from TG1
        (0b11LL << 12) | // SH0=3 inner ... .. options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable
        (0b01LL << 10) | // ORGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable
        (0b01LL << 8)  | // IRGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable
        (0b0LL  << 7)  | // EPD0  ... Translation table walk disable for translations using TTBR0_EL1  0 = walk, 1 = generate fault
        (25LL   << 0);   // T0SZ=25 (512G)  ... The region size is 2 POWER (64-T0SZ) bytes
    asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));

    ENABLEMMU();
    return;
	
	// finally, toggle some bits in system control register to enable page translation
    asm volatile ("isb; mrs %0, sctlr_el1" : "=r" (r));
    r |= 0xC00800;     // set mandatory reserved bits
    r |= (1<<0); // Only enable MMU, no caches
    /*(1<<12) |     // I, Instruction cache enable. This is an enable bit for instruction caches at EL0 and EL1
           (1<<4)  |   // SA0, tack Alignment Check Enable for EL0
           (1<<3)  |   // SA, Stack Alignment Check Enable
           (1<<2)  |   // C, Data cache enable. This is an enable bit for data caches at EL0 and EL1
           (1<<1)  |   // A, Alignment check enable bit
           (1<<0);     // set M, enable MMU*/
    //asm volatile ("msr sctlr_el1, %0; isb" : : "r" (r));
}