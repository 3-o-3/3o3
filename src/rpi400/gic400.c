/******************************************************************************
 *                        OS-3o3 operating system
 *
 *               Interrupt controller for the Raspberry PI400
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 *
 *****************************************************************************/

/* https://developer.arm.com/documentation/ddi0471/b/functional-description/functional-overview-of-the-gic-400
 https://developer.arm.com/documentation/ihi0048/b/Programmers--Model/CPU-interface-register-descriptions/Interrupt-Acknowledge-Register--GICC-IAR?lang=en
*/
#include "../../include/klib.h"

static void *irq_handlers[IRQ_LINES];
static void *irq_params[IRQ_LINES];

void gic400__irq_enable(int irq)
{
    poke(GICD_ISENABLER0 + 4 * (irq / 32), 1 << (irq % 32));
    poke(GICD_ITARGETSR0 + 4 * (irq / 4), 1 << ((irq % 4) * 8)); /* core 0 */
}

void gic400__irq_disable(int irq)
{
    poke(GICD_ICENABLER0 + 4 * (irq / 32), 1 << (irq % 32));
}

void gic400__irq_vector(void)
{
    int irq;
    int (*h)(void *);
    irq = peek(GICC_IAR) & GICC_IAR_INTERID;
    if (irq <= 15 || irq >= IRQ_LINES)
    {
        return;
    }
    h = (int (*)(void *))irq_handlers[irq];
    if (h)
    {
        h(irq_params[irq]);
    }
    else
    {
        /*gic400__irq_disable(irq);*/
    }
    poke(CEOIR, irq & CEOIR_ID_MASK);
}

void gic400__irq_init(void)
{
    int i;
    for (i = 0; i < IRQ_LINES; i++)
    {
        irq_handlers[i] = (void *)0;
        irq_params[i] = (void *)0;
    }
    sync_cache();
}

void gic400__irq_disconnect(int irq, void *handler, void *param)
{
    irq_handlers[irq] = (void *)0;
    irq_params[irq] = (void *)0;
    gic400__irq_disable(irq);
}

void gic400__irq_connect(int irq, void *handler, void *param)
{
    irq_handlers[irq] = handler;
    irq_params[irq] = param;
    gic400__irq_enable(irq);
    /*puts("connect\n");*/
}
