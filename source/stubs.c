#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "uart.h"


/* Increase program data space. As malloc and related functions depend on this,
   it is useful to have a working implementation. The following suffices for a
   standalone system; it exploits the symbol _end automatically defined by the
   GNU linker. */
caddr_t _sbrk( int incr ) {
    extern char _end;
    static char* heap_end = 0;
    char* prev_heap_end;

    if( heap_end == 0 )
        heap_end = &_end;

    char hex[10];
    memset(hex, 0, 10);

    sprintf(hex, "%x", (unsigned int)heap_end); 
    uart_puts("sbrk: ");
    uart_puts(hex);
    uart_putc('\n');

    prev_heap_end = heap_end;

    heap_end += incr;
    return (caddr_t)prev_heap_end;
}
