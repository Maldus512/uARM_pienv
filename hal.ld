/* Default linker script, for normal executables */
/*OUTPUT_ARCH(arm)*/

MEMORY {
    RAM (xrw)      : ORIGIN = 0x80000 , LENGTH = 0x8000000 /* 128MB */
}

ENTRY(_start)
SECTIONS
{
    . = 0x80000;
    __text_start = .;
    .text : {
        KEEP(*(.init))
        *(.text .text.* .gnu.linkonce.t*)
    }  > RAM
    __text_end = .;

    __rodata_start = .;
    .rodata : {
        *(.rodata .rodata.*)
    } > RAM
    __rodata_end = .;

    . = ALIGN(4096);
    __data_start = . ;
    PROVIDE(_data = .);
    .data  : {
        *(.data .data.*)
    } > RAM

    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } > RAM
    __bss_end = .;
    __end__ = .;

    .stack : {
        . = ALIGN(16);  /* Stack must always be aligned to 8 byte boundary AAPCS64 call standard */
        __stack_start__ = .;
        . = . + 65536;    /* EL0 stack size */
        . = ALIGN(16);
        __EL0_stack = .;
        PROVIDE(__EL0_stack = .);
        . = . + 16384;    /* EL1 stack size */
        . = ALIGN(16);
        __EL1_stack = .;
        PROVIDE(__EL1_stack = .);
        . = . + 4096;  /* EL2 stack size (start-up) */
        . = ALIGN(16);
        __EL2_stack = .;
        . = . + 4096;
        . = ALIGN(16);
        __stack_end__ = .;    
    } > RAM


    .heap : {
        . = ALIGN(4096);
        _page_table = .; 
        PROVIDE (page_table = .);
        . = . + 524288;
        . = ALIGN(4096); /* Room for page table */
        __heap_start__ = .;	/* Label in case we want address of heap section start */
        _end = .; 
        PROVIDE (end = .); /* Any memory from here is free to use so this is end of code and start of heap */
    } > RAM

	/*/DISCARD/ : {
		*(*)
    }*/
}
