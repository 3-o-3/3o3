
/*
 *                         OS-3o3 Operating System
 *
 *                      13 may MMXXIV PUBLIC DOMAIN
 *           The authors disclaim copyright to this source code.
 *
 *
 */

/*
 * memory range that is safe to use : 0x000500 to  0x07FFFF and 0x100000 to 0xEFFFFF
 */

#ifndef __DEFS_H__
#define __DEFS_H__ 1

#define FB_DEFAULT_ADDR 0xA0000000

#define XHCI_DEFAULT_BASE0 0xB0000000
#define XHCI_DEFAULT_BASE1 0xB8000000
#define XHCI_SUPPORTED_VERSION 0

#define XHCI_MAX_CONTROLLER 2

#define PCI_MAX_BUS 256
#define PCI_MAX_SLOT 32
#define PCI_MAX_FUNCTION 8
#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc

#define APIC_BASE_MSR 0x1B
#define APIC_BASE_MSR_BSP 0x100
#define APIC_BASE_MSR_ENABLE 0x800
#define APIC_DISABLE 0x10000
#define APIC_SW_ENABLE 0x100
#define APIC_CPUFOCUS 0x200
#define APIC_NMI (4 << 8)
#define TMR_PERIODIC 0x20000
#define TMR_BASEDIV (1 << 20)

#define APIC_APICID (0x020 >> 2)
#define APIC_APICVER (0x030 >> 2)
#define APIC_TASKPRIOR (0x080 >> 2)
#define APIC_EOI (0x0B0 >> 2)
#define APIC_LDR (0x0D0 >> 2)
#define APIC_DFR (0x0E0 >> 2)
#define APIC_SPURIOUS (0x0F0 >> 2)
#define APIC_ESR (0x280 >> 2)
#define APIC_ICRL (0x300 >> 2)
#define APIC_ICRH (0x310 >> 2)
#define APIC_LVT_TMR (0x320 >> 2)
#define APIC_LVT_PERF (0x340 >> 2)
#define APIC_LVT_LINT0 (0x350 >> 2)
#define APIC_LVT_LINT1 (0x360 >> 2)
#define APIC_LVT_ERR (0x370 >> 2)
#define APIC_TMRINITCNT (0x380 >> 2)
#define APIC_LAST (0x38F >> 2)
#define APIC_TMRCURRCNT (0x390 >> 2)
#define APIC_TMRDIV (0x3E0 >> 2)

/*#define RAM_EXTERNAL_BASE 0x00010000*/
#define RAM_EXTERNAL_SIZE ((1024 * 1024 * 8)) /* 8MB*/
#define IRQ_MASK irq_mask
#define IRQ_STATUS irq_status
#define IRQ_COUNTER18 1
#define IRQ_COUNTER18_NOT 2

#define LDADDR 0x010000                     /* io2.S _start load address*/
#define CODESIZE 1024                       /* size of the second stage code portion */
#define GFX_INFO_OFFSET (CODESIZE + 16)     /* Graphic card info buffer in io2.S */
#define GFX_INFO (LDADDR + GFX_INFO_OFFSET) /* full address*/
#define KERNEL_INFO (GFX_INFO + 0x100)      /* kernel info structure full address */
#define HPET_CONFIG (KERNEL_INFO + 0x8)
#define KERNEL_ENTRY32 (LDADDR + CODESIZE)
#define KERNEL_ENTRY64 (LDADDR + CODESIZE + 0x1300)

#define IDTR_ADDR (0x010000) /* FIXME */
#define GDT_ADDR (IDTR_ADDR + 0x20)
#define GDTR_ADDR (GDT_ADDR + 0x30)
/*#define TSS_SEG (0x28)*/

#define PDT_ADDR ((50 * 1024 * 1024) - ((4 * 512 + 4 + 1) * 0x1000)) /* FIXME */
#define IDT_ADDR (GDT_ADDR + 0x600)

#define ISR_ADDR (GDT_ADDR + 0x1800)
#define ISR_STUB (ISR_ADDR)
#define ISR_STUB_ERR (ISR_ADDR + 0x10)
#define ISR_SYSCALL (ISR_ADDR + 0x20)
#define ISR_TIMER (ISR_ADDR + 0x40)
#define ISR_KBD (ISR_ADDR + 0x60)
#define ISR_MOUSE (ISR_ADDR + 0x70)
#define ISR_USB (ISR_ADDR + 0x80)

#endif /* __DEFS_H__ */
