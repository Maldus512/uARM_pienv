file build/output.elf
target remote localhost:1234
b _start
b bios_main
b c_irq_handler
