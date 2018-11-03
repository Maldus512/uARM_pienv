#include "interrupts.h"
#include "gpio.h"
#include "mailbox.h"
#include "timers.h"
#include "uart.h"
#include "bios_const.h"
#include "asmlib.h"
#include "uARMtypes.h"
#include "libuarm.h"
#include "libuarmv2.h"


uint64_t millisecondsSinceStart  = 0;
uint64_t timeLeftToNextInterrupt = 0;


uint32_t c_swi_handler(uint32_t code, uint32_t *registers) {
    state_t *state;
    switch (code) {
        case SYS_GETCURRENTEL:
            return GETSAVEDEL();
        case SYS_GETARMCLKFRQ:
            return GETARMCLKFRQ();
        case SYS_GETARMCOUNTER:
            return GETARMCOUNTER();
        case SYS_ENABLEIRQ:
            enable_irq_el0();
            tprint("interrupts enabled!\n");
            return 0;
        case SYS_INITARMTIMER:
            initArmTimer();
            tprint("arm timer enabled!\n");
            return 0;
        case SYS_SETNEXTTIMER:
            return setTimer((unsigned int)registers);
        case SYS_GETSPEL0:
            return GETSP_EL0();
        case SYS_LAUNCHSTATE:
            state = (state_t *)registers;
            LDST_EL0(state);
            return 0;

        default:
            uart0_puts("ciao\n");
            hexstring(code);
            hexstring(GETSAVEDSTATE());
            break;
    }
    // LDELR((uint64_t)test);
    return 0;
}

void c_irq_handler() {
    static uint8_t  f_led = 0;
    uint32_t        tmp;
    static uint64_t lastBlink = 0;
    state_t *       state;
    uint64_t        timer;

    timer = getMillisecondsSinceStart();

    if (timer - lastBlink >= 1000) {
        led(f_led);
        f_led     = f_led == 0 ? 1 : 0;
        lastBlink = timer;
    }

    // check interrupt source
    tmp = *((volatile uint32_t *)CORE0_IRQ_SOURCE);

    if (tmp & 0x08) {
        disableCounter();
#ifdef APP
        // TODO: call the appropriate handler
        state = (state_t *)INT_NEWAREA;
        enable_irq();
        LDST(state);
#endif
    }
    return;
}
