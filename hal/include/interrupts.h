#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include "hardwareprofile.h"


#define INTERRUPT_CONTROLLER_BASE   ( IO_BASE + 0xB200 )

struct IRQ_CONTROLLER_REG {
    volatile uint32_t IRQ_basic_pending;
    volatile uint32_t IRQ_pending_1;
    volatile uint32_t IRQ_pending_2;
    volatile uint32_t FIQ_control;
    volatile uint32_t Enable_IRQs_1;
    volatile uint32_t Enable_IRQs_2;
    volatile uint32_t Enable_Basic_IRQs;
    volatile uint32_t Disable_IRQs_1;
    volatile uint32_t Disable_IRQs_2;
    volatile uint32_t Disable_Basic_IRQs;
};

#define GPU_INTERRUPTS_ROUTING ((volatile uint32_t *)(0x4000000C))
#define CORE0_IRQ_SOURCE ((volatile uint32_t *)(0x40000060))

#define IRQ_CONTROLLER ((struct IRQ_CONTROLLER_REG*) INTERRUPT_CONTROLLER_BASE)

void enable_irq();
void enable_irq_el0();
unsigned int setNextTimerInterrupt(unsigned int timer);

#endif