/* Default linker script, for normal executables */
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
	      "elf32-littlearm")
OUTPUT_ARCH(arm)

MEMORY {
    RAM (xrw) : ORIGIN = 0x8000, LENGTH = 0x20000000 - 0x8000000 /*500 MB - 128 MB*/
}

ENTRY(_start)
SECTIONS
{
    . = 0x8000;
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

    __bss_start = .;
    .bss : {
        *(.bss)
        //. = ALIGN(4096);
    } > RAM
    __bss_end__ = .;
    __end__ = .;
    _end = .; PROVIDE(end = .);
}