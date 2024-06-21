
/******************************************************************************
 *                        OS-3o3 operating system
 *
 *                          interrupt for i386
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 *
 *****************************************************************************/

#include "../../include/klib.h"

#include "../../include/rtos.h"

void cpu_lidt(void *idt);
void cpu_interrupt_init();
void cpu_apic_init();
void cpu_interrupt_enable(os_int32);
void delay();

struct idt_desc
{
    os_uint32 low;
    os_uint32 high;
};

struct idtr
{
    os_uint8 v[10];
};


os_uint32 tick = 0;
volatile os_uint32 inter_enable = 0;
os_uint32 *ioapic_addr = (void *)0xFEC00000;
os_uint32 *lapic_addr = (void *)0xFEE00000;

void isr_handler(struct reg_buf *regs)
{
    os_int32 n;
    n = regs->int_no;

    if (n >= 0x8 && n <= 0xF)
    {
        /* clear PIC 1 */
        out8(0x20, 0x20);
        if (lapic_addr)
        {
            lapic_addr[0xB0 >> 2] = 0;
        }
    }
    else if (n >= 0x70 && n <= 0x7F)
    {
        /* clear PIC 2 and 1 */
        out8(0xA0, 0x20);
        out8(0x20, 0x20);
    }

    if (n == 0x08)
    {
        rtos__interrupt_service_routine(1, regs);
        /*  if (Isr[0]) {
           Rtos__asm_interrupt_enable(0);
           Isr[0](regs);
          Rtos__asm_interrupt_enable(1);
         }*/

        if ((tick & 0x7F) == 0)
        {
            /*k__puts(" T ");*/
        }
        tick++;
        return;
    }
}

void idt_set(os_int32 index, os_uint32 base, os_int32 selector, os_uint8 flags)
{
    struct idt_desc *idt = (struct idt_desc *)IDT_ADDR;
    idt[index].low = ((os_uint32)base & 0xFFFF) | ((selector << 16) & 0xFFFF0000);
    idt[index].high = (((flags) << 8) & 0x0000FF00) | ((os_uint32)base & 0xFFFF0000);
}

os_uint32 ioapic_read(os_uint8 reg)
{
    ioapic_addr[0] = reg;
    return ioapic_addr[0x10 >> 2];
}

void ioapic_write(os_uint8 reg, os_uint32 val)
{
    ioapic_addr[0] = reg;
    ioapic_addr[0x10 >> 2] = val;
}

void ioapic_init()
{
    ioapic_write(0x10, 0x10000);
    ioapic_write(0x11, 0);

    ioapic_write(0x12, 0x21);
    ioapic_write(0x13, 0);

    ioapic_write(0x30, 0xA030);
    ioapic_write(0x31, 0);
}

void apic_init()
{
    os_uint32 tmp;
    os_uint32 ticks;
    os_uint32 start;

    /*disable PIC*/
    out8(0x21, 0xFF);
    out8(0xA1, 0xFF);
    out8(0xA0, 0x20);
    out8(0x20, 0x20);
    out8(0x22, 0x70);
    out8(0x23, 0);

    /*https://web.archive.org/web/20150514082645/http://www.nondot.org/sabre/os/files/MiscHW/RealtimeClockFAQ.txt */
    /*out8(0x70, 0x8B);  init RTC */
    out8(0x70, 0x0A);
    out8(0x71, (1 << 5) | 0x6);

    lapic_addr[APIC_SPURIOUS] = 0xFF | APIC_SW_ENABLE;
    lapic_addr[APIC_TMRDIV] = 0x03;

    lapic_addr[APIC_DFR] = 0xFFFFFFFF;
    tmp = lapic_addr[APIC_LDR] & 0x00FFFFFF;
    lapic_addr[APIC_LDR] = tmp | 1;
    lapic_addr[APIC_LVT_TMR] = APIC_DISABLE;
    lapic_addr[APIC_LVT_PERF] = APIC_NMI;
    lapic_addr[APIC_LVT_LINT0] = APIC_DISABLE;
    lapic_addr[APIC_LVT_LINT1] = APIC_DISABLE;
    lapic_addr[APIC_LVT_ERR] = APIC_DISABLE;
    lapic_addr[APIC_TASKPRIOR] = 0;
    lapic_addr[APIC_TMRINITCNT] = 0;

    /*lapic_addr[APIC_LVT_LINT0] = 0x08700;
     lapic_addr[APIC_LVT_LINT1] = 0x0400;*/

    lapic_addr[APIC_TMRDIV] = 0x03;
    lapic_addr[APIC_TMRINITCNT] = 0xFFFFFFFF;
    lapic_addr[APIC_LVT_TMR] = 32 | TMR_PERIODIC;
    lapic_addr[APIC_TMRDIV] = 0x03;

    out8(0x70, 0x0A); /*Wait for to be able to access the RTC*/
    while (in8(0x71) & 0x80)
        ;

    out8(0x70, 0);   /*set Real Time Clock index to the second*/
    tmp = in8(0x71); /*read value of the RTC second in BCD format*/
    out8(0x70, 0);
    while (tmp == in8(0x71))
    {
        out8(0x70, 0x0A);
        while (in8(0x71) & 0x80)
            ;
        out8(0x70, 0);
    }
    start = ticks = lapic_addr[APIC_TMRCURRCNT];
    tmp = in8(0x71);

    if (tmp == 0x59)
    {
        tmp = 0;
    }
    else
    {
        if ((tmp & 0x0F) == 0x09)
        {
            tmp = (tmp & 0xF0) + 0x10;
        }
        else
        {
            tmp++;
        }
    }

    out8(0x70, 0);
    while (tmp != in8(0x71))
    {
        out8(0x70, 0x0A);
        while (in8(0x71) & 0x80)
            ;
        ticks = lapic_addr[APIC_TMRCURRCNT];
        out8(0x70, 0);
    }

    ticks = start - lapic_addr[APIC_TMRCURRCNT];
    lapic_addr[APIC_LVT_TMR] = APIC_DISABLE;

    ticks /= 100;
    if (ticks < 16)
    {
        ticks = 16;
    }

    lapic_addr[APIC_TMRDIV] = 0x03;
    lapic_addr[APIC_TMRINITCNT] = ticks;
    lapic_addr[APIC_LVT_TMR] = 32 | TMR_PERIODIC;
    lapic_addr[APIC_TMRDIV] = 0x03;
}

static void init_vectors()
{
    os_int32 i;

    struct idtr idtr;
    /*struct idt_desc *idt = (struct idt_desc *)IDT_ADDR;*/
    inter_enable = 0;
    idtr.v[0] = (sizeof(struct idt_desc) * 256 - 1) & 0xFF;
    idtr.v[1] = ((sizeof(struct idt_desc) * 256 - 1) >> 8) & 0xFF;
    idtr.v[2] = ((os_uint32)IDT_ADDR) & 0xFF;
    idtr.v[3] = ((os_uint32)IDT_ADDR >> 8) & 0xFF;
    idtr.v[4] = ((os_uint32)IDT_ADDR >> 16) & 0xFF;
    idtr.v[5] = ((os_uint32)IDT_ADDR >> 24) & 0xFF;
    for (i = 0; i < 32; i++)
    {
        idt_set(i, ISR_STUB, 0x8, 0x8F);
    }
    for (i = 32; i < 256; i++)
    {
        idt_set(i, ISR_STUB, 0x8, 0x8E);
    }
    for (i = 32; i < 40; i++)
    {
        idt_set(i, ISR_KBD, 0x8, 0x8E);
    }
    for (i = 40; i < 48; i++)
    {
        idt_set(i, ISR_MOUSE, 0x8, 0x80);
    }
    idt_set(32, ISR_TIMER, 0x8, 0x8E);

    idt_set(0x80, ISR_SYSCALL, 0x8, 0x8E | 0x60);

    cpu_apic_init();
    cpu_lidt(&idtr);
    /* hpet[0][0] = 3;  enable HPET timer
     ioapic_init(); */
    apic_init();
}

os_uint32 interrupt__enable(os_uint32 v)
{
    os_uint32 s = inter_enable;
    cpu_interrupt_enable(v);
    inter_enable = v;
    return s;
}

void interrupt__init()
{
    init_vectors();
    interrupt__enable(0);
    interrupt__enable(1);
}
