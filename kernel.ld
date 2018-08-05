/* Default linker script, for normal executables */
/*OUTPUT_ARCH(arm)*/

MEMORY {
    RAM (xrw)      : ORIGIN = 0x80000 , LENGTH = 0x40000000 - 0x8000000 /* 1GB - 128MB */
}

ENTRY(_start)
SECTIONS
{
    . = 0x80000;
    __text_start = .;
    .text : {
        KEEP(*(.init))
        *(.text .text.*)
        //. = ALIGN(4096); /* Align to page size */
    }  > RAM
    __text_end = .;

    __rodata_start = .;
    .rodata : {
        *(.rodata)
        //. = ALIGN(4096); 
    } > RAM
    __rodata_end = .;


    __data_start = . ;
    .data  : {
        *(.data .data.*)
    } > RAM

    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss)
        //. = ALIGN(4096);
    } > RAM
    __bss_end = .;
    __end__ = .;
    _end = .; PROVIDE(end = .);
}
