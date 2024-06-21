/******************************************************************************
 *                        OS-3o3 operating system
 *
 *               Interrupt routines for the Raspberry PI400
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 *
 *****************************************************************************/

#include "../../include/klib.h"
#include "../../include/rtos.h"

volatile os_uint32 inter_enable = 0;

unsigned long long cntp_cval(os_uint32 low, os_uint32 high)
{
    return ((unsigned long long)high << 32 | low) + CLOCKHZ / HZ;
}

void os__init_timer();
void os__clear_timer();

void interrupt__timer_isr(void *p)
{
    pl011__write('T');
    rtos__interrupt_service_routine(1, p);
    os__clear_timer();
}

os_uint32 interrupt__enable(os_uint32 v)
{
    os_uint32 s = inter_enable;
    inter_enable = v;
    if (v)
    {
        os__enable_irqs();
    }
    else
    {
        os__disable_irqs();
    }
    return s;
}

void interrupt__init()
{
    dma__init();
    gic400__irq_init();
    os__init_timer();

    gic400__irq_connect(ARM_IRQLOCAL0_CNTPNS, &interrupt__timer_isr, (void *)ARM_IRQLOCAL0_CNTPNS);
    interrupt__enable(0);
    interrupt__enable(1);
}
