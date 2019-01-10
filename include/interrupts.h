#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include <stdint.h>
#include "arch.h"


#define INTERRUPT_CONTROLLER_BASE (IO_BASE + 0xB200)
#define GIC_BASE        0x40000000

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

struct GIC_REGISTERS {
    volatile uint32_t Control;
    volatile uint32_t unused0;
    volatile uint32_t Core_Timer_Prescaler;
    volatile uint32_t GPU_Interrupts_Routing;
    volatile uint32_t Performance_Monitor_Routing_Set;
    volatile uint32_t Performance_Monitor_Routing_Clear;
    volatile uint32_t unused1;
    volatile uint32_t Core_Timer_LS;
    volatile uint32_t Core_Timer_MS;
    volatile uint32_t Local_Interrupt_Routing; 
    volatile uint32_t deleted;
    volatile uint32_t Axi_Outstanding_Counters;
    volatile uint32_t Axi_Outstanding_IRQ;
    volatile uint32_t Local_Timer_Control;
    volatile uint32_t Local_Timer_Write_Flags;
    volatile uint32_t unused2;
    volatile uint32_t Core0_Timers_Interrupt_Control;
    volatile uint32_t Core1_Timers_Interrupt_Control;
    volatile uint32_t Core2_Timers_Interrupt_Control;
    volatile uint32_t Core3_Timers_Interrupt_Control;

/* Core Mailbox Interrupt control */
    volatile uint32_t Core0_Mailbox_Interrupt_Control;
    volatile uint32_t Core1_Mailbox_Interrupt_Control;
    volatile uint32_t Core2_Mailbox_Interrupt_Control;
    volatile uint32_t Core3_Mailbox_Interrupt_Control;
    volatile uint32_t Core0_IRQ_Source;
    volatile uint32_t Core1_IRQ_Source;
    volatile uint32_t Core2_IRQ_Source;
    volatile uint32_t Core3_IRQ_Source;
    volatile uint32_t Core0_FIQ_Source;
    volatile uint32_t Core1_FIQ_Source;
    volatile uint32_t Core2_FIQ_Source;
    volatile uint32_t Core3_FIQ_Source;
    volatile uint32_t Core0_MailBox0_WriteSet;
    volatile uint32_t Core0_MailBox1_WriteSet;
    volatile uint32_t Core0_MailBox2_WriteSet;
    volatile uint32_t Core0_MailBox3_WriteSet;
    volatile uint32_t Core1_MailBox0_WriteSet;
    volatile uint32_t Core1_MailBox1_WriteSet;
    volatile uint32_t Core1_MailBox2_WriteSet;
    volatile uint32_t Core1_MailBox3_WriteSet;
    volatile uint32_t Core2_MailBox0_WriteSet;
    volatile uint32_t Core2_MailBox1_WriteSet;
    volatile uint32_t Core2_MailBox2_WriteSet;
    volatile uint32_t Core2_MailBox3_WriteSet;
    volatile uint32_t Core3_MailBox0_WriteSet;
    volatile uint32_t Core3_MailBox1_WriteSet;
    volatile uint32_t Core3_MailBox2_WriteSet;
    volatile uint32_t Core3_MailBox3_WriteSet;

    volatile uint32_t Core0_MailBox0_ClearSet;
    volatile uint32_t Core0_MailBox1_ClearSet;
    volatile uint32_t Core0_MailBox2_ClearSet;
    volatile uint32_t Core0_MailBox3_ClearSet;
    volatile uint32_t Core1_MailBox0_ClearSet;
    volatile uint32_t Core1_MailBox1_ClearSet;
    volatile uint32_t Core1_MailBox2_ClearSet;
    volatile uint32_t Core1_MailBox3_ClearSet;
    volatile uint32_t Core2_MailBox0_ClearSet;
    volatile uint32_t Core2_MailBox1_ClearSet;
    volatile uint32_t Core2_MailBox2_ClearSet;
    volatile uint32_t Core2_MailBox3_ClearSet;
    volatile uint32_t Core3_MailBox0_ClearSet;
    volatile uint32_t Core3_MailBox1_ClearSet;
    volatile uint32_t Core3_MailBox2_ClearSet;
    volatile uint32_t Core3_MailBox3_ClearSet;
};


#define IRQ_CONTROLLER ((struct IRQ_CONTROLLER_REG *)INTERRUPT_CONTROLLER_BASE)
#define GIC ((struct GIC_REGISTERS *)GIC_BASE)

void         enable_irq();
void         enable_irq_el0();
void set_device_timer(uint64_t microseconds);

#endif