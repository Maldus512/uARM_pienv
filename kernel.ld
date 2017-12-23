
MEMORY {
    RAM (xrw)      : ORIGIN = 0x8000 , LENGTH = 0x40000000 - 0x8000000 /* 1GB - 128MB */
}

ENTRY (_start)
/*
 * Very simple linker script, combing the text and data sections
 * and putting them starting at address 0x800.
 */
SECTIONS {
  /* Put the code at 0x8000, leaving room for ARM and
   * the stack. It also conforms to the standard expectations.
   */
    . = 0x8000;
    __text_start = .;
    .text : {
        KEEP(*(.init))
        *(.text)
    . = ALIGN(4096); /* Align to page size */
    }  > RAM
    __text_end = .;

    __rodata_start = .;
    .rodata : {
        *(.rodata)
        . = ALIGN(4096); 
    } > RAM
    __rodata_end = .;


    __data_start = .;
    .data : {
        *(.data)
        . = ALIGN(4096); 
    } > RAM
    __data_end = .;

    __bss_start = .;
    .bss : {
        *(.data)
        . = ALIGN(4096);
    } > RAM
    __bss_end = .;
    _end = .;
}