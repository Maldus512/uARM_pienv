ENTRY(_start)
SECTIONS
{
    .stack : {
        . = ALIGN(16);  /* Stack must always be aligned to 8 byte boundary AAPCS64 call standard */
        __stack_start__ = .;
        . = . + 0x2000;    /* EL0 stack size */
        . = ALIGN(16);
        __EL1_stack_core0 = .;
        . = . + 0x2000;    /* EL1 stack size */
        . = ALIGN(16);
        __EL1_stack_core1 = .;
        . = . + 0x2000;    /* EL1 stack size */
        . = ALIGN(16);
        __EL1_stack_core2 = .;
        . = . + 0x2000;    /* EL1 stack size */
        . = ALIGN(16);
        __EL1_stack_core3 = .;
        . = . + 0x2000;
        . = ALIGN(16);
        __EL2_stack_core0 = .;
        . = . + 0x2000;
        . = ALIGN(16);
        __EL2_stack_core1 = .;
        . = . + 0x2000;
        . = ALIGN(16);
        __EL2_stack_core2 = .;
        . = . + 0x2000;
        . = ALIGN(16);
        __EL2_stack_core3 = .;
        . = ALIGN(16);
        __stack_end__ = .;    
        ASSERT((__stack_end__ < 0x70000), "Error: the HAL stack takes up too much space");
    }

    . = 0x80000;
    __text_start = .;
    .text : {
        KEEP(*(.init))
        *(.text .text.* .gnu.linkonce.t*)
    }
    __text_end = .;

    __rodata_start = .;
    .rodata : {
        *(.rodata .rodata.*)
    }
    __rodata_end = .;

    . = ALIGN(4096);
    __data_start = . ;
    PROVIDE(_data = .);
    .data  : {
        *(.data .data.*)
    }

    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
    __bss_end = .;
    __end__ = .;

    _kernel_memory_end = .; 

	/*/DISCARD/ : {
		*(*)
    }*/
}
