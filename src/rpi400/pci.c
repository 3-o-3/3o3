
/*
    Modified by O'ksi'D for 32gears OS.
original at https://sourceforge.net/projects/pdos/

*/

/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pci.c - PCI functions                                            */
/*                                                                   */
/*********************************************************************/

#include "../../include/pci.h"
#include "../../include/klib.h"
#include "../../include/rtos.h"

#define ARM_PCIE_PCI_ADDR 0xf8000000ULL
#define ARM_PCIE_CPU_ADDR 0x600000000ULL
#define ARM_PCIE_ADDR_SIZE 0x4000000UL
#define BRCM_PCIE_CAP_REGS 0x00ac
#define PCI_EXP_RTCTL 28
#define PCI_EXP_RTCTL_CRSSVE 0x0010
#define PCI_COMMAND 0x04

#define ARM_PCIE_REG_ECAM (ARM_PCIE_HOST_BASE + 0x0)
#define ARM_PCIE_REG_ID (ARM_PCIE_HOST_BASE + 0x043C)
#define ARM_PCIE_REG_MEM_PCI_LO (ARM_PCIE_HOST_BASE + 0x400C)
#define ARM_PCIE_REG_MEM_PCI_HI (ARM_PCIE_HOST_BASE + 0x4010)
#define ARM_PCIE_REG_STATUS (ARM_PCIE_HOST_BASE + 0x4068)
#define ARM_PCIE_REG_REV (ARM_PCIE_HOST_BASE + 0x406C)
#define ARM_PCIE_REG_MEM_CPU_LO (ARM_PCIE_HOST_BASE + 0x4070)
#define ARM_PCIE_REG_MEM_CPU_HI_START (ARM_PCIE_HOST_BASE + 0x4080)
#define ARM_PCIE_REG_MEM_CPU_HI_END (ARM_PCIE_HOST_BASE + 0x4084)
#define ARM_PCIE_REG_DEBUG (ARM_PCIE_HOST_BASE + 0x4204)
#define ARM_PCIE_REG_INTMASK (ARM_PCIE_HOST_BASE + 0x4310)
#define ARM_PCIE_REG_INTCLR (ARM_PCIE_HOST_BASE + 0x4314)
#define ARM_PCIE_REG_CFG_INDEX (ARM_PCIE_HOST_BASE + 0x9000)
#define ARM_PCIE_REG_INIT (ARM_PCIE_HOST_BASE + 0x9210)
#define ARM_PCIE_REG_CFG_DATA (ARM_PCIE_HOST_BASE + 0x8000)

typedef struct
{
    struct
    {
        os_uint16 vid;
        os_uint16 did;
        os_uint16 command;
        os_uint16 status;
        os_uint8 revision;
        os_uint8 prog;
        os_uint8 subclass;
        os_uint8 class_;
        os_uint8 cache_line_size;
        os_uint8 latency_timer;
        os_uint8 header_type;
        os_uint8 bist;
    } common;
    os_uint32 bar[2];
    os_uint8 primary_bus;
    os_uint8 secondary_bus;
    os_uint8 subordinate_bus;
    os_uint8 secondary_latency_timer;
} PciHeader1;

volatile static os_uint8 *cfg_mem = (volatile os_uint8 *)(ARM_PCIE_REG_CFG_DATA);

static os_uint64 Win__cpu_addr = MEM_PCIE_RANGE_START;
static os_uint64 Win__pcie_addr = MEM_PCIE_RANGE_PCIE_START;
static os_uint64 Win__size = MEM_PCIE_RANGE_SIZE;
static os_uint64 Dma__pcie_addr = MEM_PCIE_DMA_RANGE_PCIE_START;
/*static os_uint64 Dma__cpu_addr = MEM_PCIE_DMA_RANGE_START;*/
static os_uint64 Dma__size = 1024 * 1024 * 1024;

static os_uint8 in8(os_intn port)
{
    return *(os_uint8 volatile *)port;
}
static os_uint16 in16(os_intn port)
{
    return *(os_uint16 volatile *)port;
}
static os_uint32 in32(os_intn port)
{
    return *(os_uint32 volatile *)port;
}
static void out8(os_intn port, os_intn data)
{
    *(os_uint8 volatile *)port = data & 0xFF;
}
static void out16(os_intn port, os_intn data)
{
    *(os_uint16 volatile *)port = data & 0xFFFF;
}
static void out32(os_intn port, os_intn data)
{
    *(os_uint32 volatile *)port = data & 0xFFFFFFFF;
}

/*
https://forums.raspberrypi.com/viewtopic.php?t=251335
*/
os_intn pci__init_()
{
    os_uint32 rev, ccode;
    os_uint32 status;
    os_uint8 cp;
    os_intn i;
    os_uint32 bus = 0;
    os_uint32 slot = 0;
    os_uint32 function = 0;
    os_uint64 cpu_addr_start = ARM_PCIE_CPU_ADDR;
    os_uint64 cpu_addr_end = ARM_PCIE_CPU_ADDR + ARM_PCIE_ADDR_SIZE;
    os_uint32 cpu_addr_low = (((ARM_PCIE_CPU_ADDR >> 16) & 0xffff) | ((((ARM_PCIE_CPU_ADDR + ARM_PCIE_ADDR_SIZE) >> 20) - 1) << 20)) & 0xFFFFFFFF;
    volatile PciHeader1 *bridge = (volatile void *)ARM_PCIE_REG_ECAM;
    os_uint32 init;
    (void)rev;
    (void)bus;
    (void)slot;
    (void)function;

    /* reset controller */
    init = in32(ARM_PCIE_REG_INIT);
    init |= 0x3;
    out32(ARM_PCIE_REG_INIT, init);
    in32(ARM_PCIE_REG_INIT);

    k__usleep(1000);

    init = in32(ARM_PCIE_REG_INIT);
    init &= ~0x2;
    out32(ARM_PCIE_REG_INIT, init);
    in32(ARM_PCIE_REG_INIT);

    rev = in32(ARM_PCIE_REG_REV);
    

    /* Clear and mask interrupts*/
    out32(ARM_PCIE_REG_INTCLR, 0xffffffff);
    out32(ARM_PCIE_REG_INTMASK, 0xffffffff);

    init = in32(ARM_PCIE_REG_INIT);
    init &= ~0x1;
    out32(ARM_PCIE_REG_INIT, init);

    /* wait for link to become active */
    status = in32(ARM_PCIE_REG_STATUS);
    for (i = 0; i < 100; i++)
    {
        if ((status & 0x30) == 0x30)
        {
            break;
        }
        k__usleep(1000);
        status = in32(ARM_PCIE_REG_STATUS);
    }

    if ((status & 0x30) != 0x30 || (status & 0x80) != 0x80)
    {
        k__printf("PCIe failure 'status=%x'\n", status);
        return -1;
    }

    /* class code */
    ccode = in32(ARM_PCIE_REG_ID);
    if ((ccode & 0xffffff) != 0x060400)
    {
        ccode = (ccode & ~0xffffff) | 0x060400;
        out32(ARM_PCIE_REG_ID, ccode);
    }

    /* set up the PCI address */
    out32(ARM_PCIE_REG_MEM_PCI_LO, (os_uint32)ARM_PCIE_PCI_ADDR);
    out32(ARM_PCIE_REG_MEM_PCI_HI, (os_uint32)(ARM_PCIE_PCI_ADDR >> 32));

    /* set up the CPU addresses */
    out32(ARM_PCIE_REG_MEM_CPU_LO, cpu_addr_low);
    out32(ARM_PCIE_REG_MEM_CPU_HI_START, cpu_addr_start >> 32);
    out32(ARM_PCIE_REG_MEM_CPU_HI_END, cpu_addr_end >> 32);

    /* Device on 0:0:0 should be a bridge */
    if (bridge->common.header_type != 1)
    {
        k__printf("PCIe bridge not found\n");
        return -1;
    }

    /* configure secondary and subordinate device numbers */
    bridge->secondary_bus = 1;
    bridge->subordinate_bus = 1;

    out16(ARM_PCIE_HOST_BASE + 0x20, MEM_PCIE_RANGE_PCIE_START >> 16); /* memory base*/
    out16(ARM_PCIE_HOST_BASE + 0x22, MEM_PCIE_RANGE_PCIE_START >> 16); /*memory limit*/
    out8(ARM_PCIE_HOST_BASE + 0x3e, PCI_BRIDGE_CTL_PARITY);            /*pci__cfg_write_bridge_control*/
    cp = in8(ARM_PCIE_HOST_BASE + BRCM_PCIE_CAP_REGS);
    if (cp != PCI_CAPABILITY_PCI_EXPRESS)
    {
        k__printf("PCI_CAPABILITY_PCI_EXPRESS failure %x %x %x\n", cp);
        return -1;
    }
    out8(ARM_PCIE_HOST_BASE + BRCM_PCIE_CAP_REGS + PCI_EXP_RTCTL,
         PCI_EXP_RTCTL_CRSSVE);
    out16(ARM_PCIE_HOST_BASE + PCI_COMMAND, PCI_COMMAND_MEMORY_SPACE | PCI_COMMAND_BUS_MASTER | PCI_COMMAND_PARITY_ERROR_RESPONSE | PCI_COMMAND_SERR_ENABLE);

    k__printf("PCI bus initialized\n");
    return 0;
}

void pci__set_addr(os_uint32 bus, os_uint32 slot,
                   os_uint32 function, os_uint32 offset)
{
    os_uint32 address;
    address = ((os_uint32)(bus & 0xFF) << 20) | ((os_uint32)(slot & 0x1F) << 15) | ((os_uint32)(function & 0x7) << 12);
    out32(ARM_PCIE_REG_CFG_INDEX, address);
}

os_uint8 pci__cfg_read8(os_uint32 bus, os_uint32 slot,
                        os_uint32 function, os_uint32 offset)
{
    pci__set_addr(bus, slot, function, offset);
    return cfg_mem[offset];
}

os_uint16 pci__cfg_read16(os_uint32 bus, os_uint32 slot,
                          os_uint32 function, os_uint32 offset)
{
    pci__set_addr(bus, slot, function, offset);
    return ((os_uint16 *)(cfg_mem + offset))[0];
}

os_uint32 pci__cfg_read32(os_uint32 bus, os_uint32 slot,
                          os_uint32 function, os_uint32 offset)
{
    pci__set_addr(bus, slot, function, offset);
    return ((os_uint32 *)(cfg_mem + offset))[0];
}

os_uint8 pci__cfg_write8(os_uint32 bus, os_uint32 slot,
                         os_uint32 function, os_uint32 offset,
                         os_uint8 data)
{
    pci__set_addr(bus, slot, function, offset);
    *((os_uint8 *)(cfg_mem + offset)) = data;
    return *((os_uint8 *)(cfg_mem + offset));
}

os_uint16 pci__cfg_write16(os_uint32 bus, os_uint32 slot,
                           os_uint32 function, os_uint32 offset,
                           os_uint32 data)
{
    pci__set_addr(bus, slot, function, offset);
    *((os_uint16 *)(cfg_mem + offset)) = data;
    return *((os_uint16 *)(cfg_mem + offset));
}

os_uint32 pci__cfg_write32(os_uint32 bus, os_uint32 slot,
                           os_uint32 function, os_uint32 offset,
                           os_size data)
{
    pci__set_addr(bus, slot, function, offset);
    *((os_uint32 *)(cfg_mem + offset)) = data;
    return *((os_uint32 *)(cfg_mem + offset));
}

os_intn pci__find_device(os_uint32 vendor, os_uint32 device,
                         os_uint32 index, os_uint32 *bus,
                         os_uint32 *slot, os_uint32 *function)
{
    os_uint32 read_vendor;
    os_uint8 read_header;

    for (*bus = 0; *bus < PCI_MAX_BUS; (*bus)++)
    {
        for (*slot = 0; *slot < PCI_MAX_SLOT; (*slot)++)
        {
            read_vendor = pci__cfg_read_vendor(*bus, *slot, 0);
            if (read_vendor != PCI_VENDOR_NO_DEVICE)
            {
                /* In case it is a multifunction device,
                 * all functions must always be checked
                 * as they do not have to be in order. */
                read_header = pci__cfg_read_header_type(*bus, *slot, 0);
                if (read_header & PCI_HEADER_TYPE_MULTI_FUNCTION)
                {
                    for (*function = 0;
                         *function < PCI_MAX_FUNCTION;
                         (*function)++)
                    {
                        if (pci__cfg_read_vendor(*bus, *slot, *function) == vendor)
                        {
                            if (pci__cfg_read_device(*bus, *slot, *function) == device)
                            {
                                if (index == 0)
                                    return (0);
                                index--;
                            }
                        }
                    }
                }
                /* Otherwise it must be a single function device
                 * and we check only the first function. */
                else if (read_vendor == vendor)
                {
                    if (pci__cfg_read_device(*bus, *slot, 0) == device)
                    {
                        if (index == 0)
                        {
                            *function = 0;
                            return (0);
                        }
                        index--;
                    }
                }
            }
        }
    }
    return (1);
}

os_intn pci__find_class_code(os_uint8 class_, os_uint8 subclass,
                             os_uint8 prog_IF, os_uint32 index,
                             os_uint32 *bus, os_uint32 *slot,
                             os_uint32 *function)
{
    os_uint32 max_function;
    for (*bus = 0; *bus < PCI_MAX_BUS; (*bus)++)
    {
        for (*slot = 0; *slot < PCI_MAX_SLOT; (*slot)++)
        {
            if (pci__cfg_read_vendor(*bus, *slot, 0) != PCI_VENDOR_NO_DEVICE)
            {
                /* In case it is a multifunction device,
                 * all functions must always be checked
                 * as they do not have to be in order. */
                if (pci__cfg_read_header_type(*bus, *slot, 0) & PCI_HEADER_TYPE_MULTI_FUNCTION)
                {
                    max_function = PCI_MAX_FUNCTION;
                }
                /* Otherwise it must be a single function device
                 * and we check only the first function. */
                else
                    max_function = 1;
                for (*function = 0;
                     *function < max_function;
                     (*function)++)
                {
                    if (class_ == PCI_CLASS_WILDCARD || (pci__cfg_read_class(*bus, *slot, *function) == class_))
                    {
                        if (subclass == PCI_SUBCLASS_WILDCARD || (pci__cfg_read_subclass(*bus, *slot, *function) == subclass))
                        {
                            if (prog_IF == PCI_PROG_IF_WILDCARD || (pci__cfg_read_prog_if(*bus, *slot, *function) == prog_IF))
                            {
                                if (index == 0)
                                    return (0);
                                index--;
                            }
                        }
                    }
                }
            }
        }
    }
    return (1);
}

/*https://github.com/fysnet/FYSOS/blob/master/main/usb/utils/include/pci.h
*/
os_uint32 pci__mem_range(os_uint8 bus,
                         os_uint8 dev,
                         os_uint8 func,
                         os_uint8 offset)
{
    os_size org0, org1, cmnd;
    os_size range[2];

    cmnd = pciConfigReadCommand(bus, dev, func);
    pci__cfg_write_command(bus, dev, func, cmnd & ~0x07);

    org0 = pci__cfg_read32(bus, dev, func, offset);
    if ((org0 & 0x07) == PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE)
    {
        org1 = pci__cfg_read32(bus, dev, func, offset + 4);
    }

    pci__cfg_write32(bus, dev, func, offset, 0xFFFFFFFF);
    if ((org0 & 0x07) == PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE)
    {
        pci__cfg_write32(bus, dev, func, offset + 4, 0xFFFFFFFF);
    }

    range[0] = pci__cfg_read32(bus, dev, func, offset);
    range[1] = 0;
    if ((org0 & 0x07) == PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE)
    {
        range[1] = pci__cfg_read32(bus, dev, func, offset + 4);
    }

    pci__cfg_write32(bus, dev, func, offset, org0);
    if ((org0 & 0x07) == PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE)
    {
        pci__cfg_write32(bus, dev, func, offset + 4, org1);
    }

    pci__cfg_write_command(bus, dev, func, cmnd);

    if (org0 & PCI_BASE_ADDRESS_IO_SPACE)
    {
        org0 = range[0];
        if ((org0 & 0xFFFF0000) == 0)
        {
            org0 |= 0xFFFF0000;
        }
        return (os_size)(~(org0 & ~0x1) + 1);
    }
    else
    {
        return (~(range[0] & ~0xF) + 1);
    }
}

os_uint32 cfg_index(os_uint32 bus, os_uint32 slot, os_uint32 function, os_uint32 reg)
{
    return ((slot & 0x1f) << 15) | ((function & 0x07) << 12) | (bus << 20) | (reg & ~3);
}

#define PCIE_GEN 2

#define BRCM_PCIE_CAP_REGS 0x00ac

#define PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1 0x0188
#define PCIE_RC_CFG_PRIV1_ID_VAL3 0x043c
#define PCIE_RC_DL_MDIO_ADDR 0x1100
#define PCIE_RC_DL_MDIO_WR_DATA 0x1104
#define PCIE_RC_DL_MDIO_RD_DATA 0x1108
#define PCIE_MISC_MISC_CTRL 0x4008
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO 0x400c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI 0x4010
#define PCIE_MISC_RC_BAR1_CONFIG_LO 0x402c
#define PCIE_MISC_RC_BAR1_CONFIG_HI 0x4030
#define PCIE_MISC_RC_BAR2_CONFIG_LO 0x4034
#define PCIE_MISC_RC_BAR2_CONFIG_HI 0x4038
#define PCIE_MISC_RC_BAR3_CONFIG_LO 0x403c
#define PCIE_MISC_MSI_BAR_CONFIG_LO 0x4044
#define PCIE_MISC_MSI_BAR_CONFIG_HI 0x4048
#define PCIE_MISC_MSI_DATA_CONFIG 0x404c
#define PCIE_MISC_EOI_CTRL 0x4060
#define PCIE_MISC_PCIE_CTRL 0x4064
#define PCIE_MISC_PCIE_STATUS 0x4068
#define PCIE_MISC_REVISION 0x406c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT 0x4070
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI 0x4080
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI 0x4084
#define PCIE_MISC_HARD_PCIE_HARD_DEBUG 0x4204
#define PCIE_INTR2_CPU_BASE 0x4300
#define PCIE_MSI_INTR2_BASE 0x4500
#define PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2_MASK 0xc
#define PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2_SHIFT 0x2
#define PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE_MASK 0xffffff
#define PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE_SHIFT 0x0
#define PCIE_MISC_MISC_CTRL_RCB_MPS_MODE_MASK 0x400
#define PCIE_MISC_MISC_CTRL_RCB_MPS_MODE_SHIFT 0xa
#define PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN_MASK 0x1000
#define PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN_SHIFT 0xc
#define PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE_MASK 0x2000
#define PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE_SHIFT 0xd
#define PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK 0x300000
#define PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_SHIFT 0x14
#define PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK 0xf8000000
#define PCIE_MISC_MISC_CTRL_SCB0_SIZE_SHIFT 0x1b
#define PCIE_MISC_MISC_CTRL_SCB1_SIZE_MASK 0x7c00000
#define PCIE_MISC_MISC_CTRL_SCB1_SIZE_SHIFT 0x16
#define PCIE_MISC_MISC_CTRL_SCB2_SIZE_MASK 0x1f
#define PCIE_MISC_MISC_CTRL_SCB2_SIZE_SHIFT 0x0
#define PCIE_MISC_RC_BAR1_CONFIG_LO_SIZE_MASK 0x1f
#define PCIE_MISC_RC_BAR1_CONFIG_LO_SIZE_SHIFT 0x0
#define PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_MASK 0x1f
#define PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_SHIFT 0x0
#define PCIE_MISC_RC_BAR3_CONFIG_LO_SIZE_MASK 0x1f
#define PCIE_MISC_RC_BAR3_CONFIG_LO_SIZE_SHIFT 0x0
#define PCIE_MISC_PCIE_CTRL_PCIE_PERSTB_MASK 0x4
#define PCIE_MISC_PCIE_CTRL_PCIE_PERSTB_SHIFT 0x2
#define PCIE_MISC_PCIE_CTRL_PCIE_L23_REQUEST_MASK 0x1
#define PCIE_MISC_PCIE_CTRL_PCIE_L23_REQUEST_SHIFT 0x0
#define PCIE_MISC_PCIE_STATUS_PCIE_PORT_MASK 0x80
#define PCIE_MISC_PCIE_STATUS_PCIE_PORT_SHIFT 0x7
#define PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE_MASK 0x20
#define PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE_SHIFT 0x5
#define PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP_MASK 0x10
#define PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP_SHIFT 0x4
#define PCIE_MISC_PCIE_STATUS_PCIE_LINK_IN_L23_MASK 0x40
#define PCIE_MISC_PCIE_STATUS_PCIE_LINK_IN_L23_SHIFT 0x6
#define PCIE_MISC_REVISION_MAJMIN_MASK 0xffff
#define PCIE_MISC_REVISION_MAJMIN_SHIFT 0
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT_MASK 0xfff00000
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT_SHIFT 0x14
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE_MASK 0xfff0
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE_SHIFT 0x4
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_NUM_MASK_BITS 0xc
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI_BASE_MASK 0xff
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI_BASE_SHIFT 0x0
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI_LIMIT_MASK 0xff
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI_LIMIT_SHIFT 0x0
#define PCIE_MISC_HARD_PCIE_HARD_DEBUG_CLKREQ_DEBUG_ENABLE_MASK 0x2
#define PCIE_MISC_HARD_PCIE_HARD_DEBUG_CLKREQ_DEBUG_ENABLE_SHIFT 0x1
#define PCIE_MISC_HARD_PCIE_HARD_DEBUG_SERDES_IDDQ_MASK 0x08000000
#define PCIE_MISC_HARD_PCIE_HARD_DEBUG_SERDES_IDDQ_SHIFT 0x1b
#define PCIE_MISC_HARD_PCIE_HARD_DEBUG_CLKREQ_L1SS_ENABLE_MASK 0x00200000
#define PCIE_RGR1_SW_INIT_1_PERST_MASK 0x1
#define PCIE_RGR1_SW_INIT_1_PERST_SHIFT 0x0

#define PCI_EXP_LNKCAP 12                    /* Link Capabilities */
#define PCI_EXP_LNKCAP_SLS 0x0000000f        /* Supported Link Speeds */
#define PCI_EXP_LNKCAP_SLS_2_5GB 0x00000001  /* LNKCAP2 SLS Vector bit 0 */
#define PCI_EXP_LNKCAP_SLS_5_0GB 0x00000002  /* LNKCAP2 SLS Vector bit 1 */
#define PCI_EXP_LNKCAP_SLS_8_0GB 0x00000003  /* LNKCAP2 SLS Vector bit 2 */
#define PCI_EXP_LNKCAP_SLS_16_0GB 0x00000004 /* LNKCAP2 SLS Vector bit 3 */
#define PCI_EXP_LNKCAP_MLW 0x000003f0        /* Maximum Link Width */
#define PCI_EXP_LNKCAP_ASPMS 0x00000c00      /* ASPM Support */
#define PCI_EXP_LNKCAP_L0SEL 0x00007000      /* L0s Exit Latency */
#define PCI_EXP_LNKCAP_L1EL 0x00038000       /* L1 Exit Latency */
#define PCI_EXP_LNKCAP_CLKPM 0x00040000      /* Clock Power Management */
#define PCI_EXP_LNKCAP_SDERC 0x00080000      /* Surprise Down Error Reporting Capable */
#define PCI_EXP_LNKCAP_DLLLARC 0x00100000    /* Data Link Layer Link Active Reporting Capable */
#define PCI_EXP_LNKCAP_LBNC 0x00200000       /* Link Bandwidth Notification Capability */
#define PCI_EXP_LNKCAP_PN 0xff000000         /* Port Number */
#define PCI_EXP_LNKCTL2 48                   /* Link Control 2 */
#define PCI_EXP_LNKCTL2_TLS 0x000f
#define PCI_EXP_LNKCTL2_TLS_2_5GT 0x0001  /* Supported Speed 2.5GT/s */
#define PCI_EXP_LNKCTL2_TLS_5_0GT 0x0002  /* Supported Speed 5GT/s */
#define PCI_EXP_LNKCTL2_TLS_8_0GT 0x0003  /* Supported Speed 8GT/s */
#define PCI_EXP_LNKCTL2_TLS_16_0GT 0x0004 /* Supported Speed 16GT/s */

#define PCI_EXP_LNKSTA 18                /* Link Status */
#define PCI_EXP_LNKSTA_CLS 0x000f        /* Current Link Speed */
#define PCI_EXP_LNKSTA_CLS_2_5GB 0x0001  /* Current Link Speed 2.5GT/s */
#define PCI_EXP_LNKSTA_CLS_5_0GB 0x0002  /* Current Link Speed 5.0GT/s */
#define PCI_EXP_LNKSTA_CLS_8_0GB 0x0003  /* Current Link Speed 8.0GT/s */
#define PCI_EXP_LNKSTA_CLS_16_0GB 0x0004 /* Current Link Speed 16.0GT/s */
#define PCI_EXP_LNKSTA_NLW 0x03f0        /* Negotiated Link Width */
#define PCI_EXP_LNKSTA_NLW_X1 0x0010     /* Current Link Width x1 */
#define PCI_EXP_LNKSTA_NLW_X2 0x0020     /* Current Link Width x2 */
#define PCI_EXP_LNKSTA_NLW_X4 0x0040     /* Current Link Width x4 */
#define PCI_EXP_LNKSTA_NLW_X8 0x0080     /* Current Link Width x8 */
#define PCI_EXP_LNKSTA_NLW_SHIFT 4       /* start of NLW mask in link status */
#define PCI_EXP_LNKSTA_LT 0x0800         /* Link Training */
#define PCI_EXP_LNKSTA_SLC 0x1000        /* Slot Clock Configuration */
#define PCI_EXP_LNKSTA_DLLLA 0x2000      /* Data Link Layer Link Active */
#define PCI_EXP_LNKSTA_LBMS 0x4000       /* Link Bandwidth Management Status */
#define PCI_EXP_LNKSTA_LABS 0x8000       /* Link Autonomous Bandwidth Status */

#define STATUS 0x0
#define SET 0x4
#define CLR 0x8
#define MASK_STATUS 0xc
#define MASK_SET 0x10
#define MASK_CLR 0x14

#define DATA_ENDIAN 0
#define MMIO_ENDIAN 0

#define BURST_SIZE_128 0
#define BURST_SIZE_256 1
#define BURST_SIZE_512 2

#define BRCM_INT_PCI_MSI_NR 32
#define BRCM_PCIE_HW_REV_33 0x0303

#define PCIE_RGR1_SW_INIT_1 0x9210
#define RGR1_SW_INIT_1_INIT_GENERIC_MASK 0x2
#define RGR1_SW_INIT_1_INIT_GENERIC_SHIFT 0x1
#define PCIE_EXT_CFG_INDEX 0x9000
#define PCIE_EXT_CFG_DATA 0x8000

#define bcm_readl(a) in32(a)
#define bcm_writel(d, a) out32(a, d)
#define bcm_readw(a) in16(a)
#define bcm_writew(d, a) out16(a, d)

/* These macros extract/insert fields to host controller's register set. */
#define RD_FLD(m_base, reg, field) \
    rd_fld(m_base + reg, reg##_##field##_MASK, reg##_##field##_SHIFT)
#define WR_FLD(m_base, reg, field, val) \
    wr_fld(m_base + reg, reg##_##field##_MASK, reg##_##field##_SHIFT, val)
#define WR_FLD_RB(m_base, reg, field, val) \
    wr_fld_rb(m_base + reg, reg##_##field##_MASK, reg##_##field##_SHIFT, val)
#define WR_FLD_WITH_OFFSET(m_base, off, reg, field, val) \
    wr_fld(m_base + reg + off, reg##_##field##_MASK, reg##_##field##_SHIFT, val)
#define EXTRACT_FIELD(val, reg, field) \
    ((val & reg##_##field##_MASK) >> reg##_##field##_SHIFT)
#define INSERT_FIELD(val, reg, field, field_val) \
    ((val & ~reg##_##field##_MASK) | (reg##_##field##_MASK & (field_val << reg##_##field##_SHIFT)))

os_uint32 rd_fld(os_uint32 p, os_uint32 mask, int shift)
{
    return (bcm_readl(p) & mask) >> shift;
}

void wr_fld(os_uint32 p, os_uint32 mask, int shift, os_uint32 val)
{
    os_uint32 reg = bcm_readl(p);

    reg = (reg & ~mask) | ((val << shift) & mask);
    bcm_writel(reg, p);
}

void wr_fld_rb(os_uint32 p, os_uint32 mask, int shift, os_uint32 val)
{
    wr_fld(p, mask, shift, val);
    (void)bcm_readl(p);
}

static int encode_ibar_size(os_uint64 size)
{
    int log2_in = 0;

    while (size > 0)
    {
        log2_in++;
        size >>= 1;
    }
    if (log2_in >= 12 && log2_in <= 15)
        /* Covers 4KB to 32KB (inclusive) */
        return (log2_in - 12) + 0x1c;
    else if (log2_in >= 16 && log2_in <= 37)
        /* Covers 64KB to 32GB, (inclusive) */
        return log2_in - 15;
    /* Something is awry so disable */
    return 0;
}
os_intn pcie__link_up(os_uint32 base)
{
    os_uint32 val = bcm_readl(base + PCIE_MISC_PCIE_STATUS);
    os_uint32 dla = EXTRACT_FIELD(val, PCIE_MISC_PCIE_STATUS, PCIE_DL_ACTIVE);
    os_uint32 plu = EXTRACT_FIELD(val, PCIE_MISC_PCIE_STATUS, PCIE_PHYLINKUP);

    return (dla && plu) ? 1 : 0;
}

/* The controller is capable of serving in both RC and EP roles */
os_intn pcie__rc_mode(os_uint32 base)
{
    os_uint32 val = bcm_readl(base + PCIE_MISC_PCIE_STATUS);

    return !!EXTRACT_FIELD(val, PCIE_MISC_PCIE_STATUS, PCIE_PORT);
}

void pcie_set_outbound_win(os_uint32 base, os_uint32 win, os_uint64 cpu_addr,
                           os_uint64 pcie_addr, os_uint64 size)
{
    os_uint64 cpu_addr_mb, limit_addr_mb;
    os_uint32 tmp;

    /* Set the m_base of the pcie_addr window */
    bcm_writel(pcie_addr + MMIO_ENDIAN,
               base + PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO + (win * 8));
    bcm_writel(0,
               base + PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI + (win * 8));

    cpu_addr_mb = cpu_addr >> 20;
    limit_addr_mb = (cpu_addr + size - 1) >> 20;

    /* Write the addr m_base low register */
    WR_FLD_WITH_OFFSET(base, (win * 4),
                       PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT,
                       BASE, cpu_addr_mb);
    /* Write the addr limit low register */
    WR_FLD_WITH_OFFSET(base, (win * 4),
                       PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT,
                       LIMIT, limit_addr_mb);

    /* Write the cpu addr high register */
    tmp = (os_uint32)(cpu_addr_mb >> PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_NUM_MASK_BITS);
    WR_FLD_WITH_OFFSET(base, (win * 8),
                       PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI,
                       BASE, tmp);
    /* Write the cpu limit high register */
    tmp = (os_uint32)(limit_addr_mb >> PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_NUM_MASK_BITS);
    WR_FLD_WITH_OFFSET(base, (win * 8),
                       PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI,
                       LIMIT, tmp);
}

os_intn pci__init()
{
    os_uint32 bus = 0;
    os_uint32 slot = 0;
    os_uint32 function = 0;
    os_uint32 base = ARM_PCIE_HOST_BASE;
    os_uint32 rev;
    os_uint8 ht;
    os_uint8 cp;
    os_uint32 tmp;
    (void)bus;
    (void)slot;
    (void)function;
    
    /* Reset the bridge */
    wr_fld_rb(base + PCIE_RGR1_SW_INIT_1, RGR1_SW_INIT_1_INIT_GENERIC_MASK, RGR1_SW_INIT_1_INIT_GENERIC_SHIFT, 1);
    wr_fld_rb(base + PCIE_RGR1_SW_INIT_1, PCIE_RGR1_SW_INIT_1_PERST_MASK,
              PCIE_RGR1_SW_INIT_1_PERST_SHIFT, 1);
    k__usleep(200);
    wr_fld_rb(base + PCIE_RGR1_SW_INIT_1, RGR1_SW_INIT_1_INIT_GENERIC_MASK, RGR1_SW_INIT_1_INIT_GENERIC_SHIFT, 0);
    k__usleep(200);
    WR_FLD_RB(base, PCIE_MISC_HARD_PCIE_HARD_DEBUG, SERDES_IDDQ, 0);

    tmp = bcm_readl(base + PCIE_MISC_REVISION);
    rev = EXTRACT_FIELD(tmp, PCIE_MISC_REVISION, MAJMIN);

    tmp = bcm_readl(base + PCIE_MISC_MISC_CTRL);
    tmp = INSERT_FIELD(tmp, PCIE_MISC_MISC_CTRL, SCB_ACCESS_EN, 1);
    tmp = INSERT_FIELD(tmp, PCIE_MISC_MISC_CTRL, CFG_READ_UR_MODE, 1);
    tmp = INSERT_FIELD(tmp, PCIE_MISC_MISC_CTRL, MAX_BURST_SIZE, BURST_SIZE_128);
    bcm_writel(tmp, base + PCIE_MISC_MISC_CTRL);
#if 0
    static os_uint64 Win__cpu_addr = MEM_PCIE_RANGE_START;
    static os_uint64 Win__pcie_addr = MEM_PCIE_RANGE_PCIE_START;
    static os_uint64 Win__size = MEM_PCIE_RANGE_SIZE;
    static os_uint64 Dma__pcie_addr = MEM_PCIE_DMA_RANGE_PCIE_START;
    static os_uint64 Dma__cpu_addr = MEM_PCIE_DMA_RANGE_START;
    static os_uint64 Dma__size = 1024 * 1024 * 1024;



    m_scb_size[0] = Dma__size;		// must be a power of 2
    m_num_scbs = 1;
    os_uint32 rc_bar2_offset =Dma__pcie_addr;

    os_uint32 total_mem_size = Dma__size;
    os_uint32 rc_bar2_size = Dma__size;			// must be a power of 2

    os_uint64 msi_target_addr = BRCM_MSI_TARGET_ADDR_LT_4GB;
    
    m_msi_target_addr = BRCM_MSI_TARGET_ADDR_LT_4GB;
#endif

    tmp = Dma__pcie_addr;
    tmp = INSERT_FIELD(tmp, PCIE_MISC_RC_BAR2_CONFIG_LO, SIZE, encode_ibar_size(Dma__size));
    bcm_writel(tmp, base + PCIE_MISC_RC_BAR2_CONFIG_LO);
    bcm_writel(0, base + PCIE_MISC_RC_BAR2_CONFIG_HI);

    WR_FLD(base, PCIE_MISC_MISC_CTRL, SCB0_SIZE, 0xf); // 1GB

    WR_FLD(base, PCIE_MISC_RC_BAR1_CONFIG_LO, SIZE, 0);

    WR_FLD(base, PCIE_MISC_RC_BAR3_CONFIG_LO, SIZE, 0);

    bcm_writel(0xffffffff, base + PCIE_INTR2_CPU_BASE + CLR);
    (void)bcm_readl(base + PCIE_INTR2_CPU_BASE + CLR);

    bcm_writel(0xffffffff, base + PCIE_INTR2_CPU_BASE + MASK_SET);
    (void)bcm_readl(base + PCIE_INTR2_CPU_BASE + MASK_SET);

    os_uint32 lnkcap = bcm_readl(base + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKCAP);
    os_uint16 lnkctl2 = bcm_readw(base + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKCTL2);

    lnkcap = (lnkcap & ~PCI_EXP_LNKCAP_SLS) | PCIE_GEN;
    bcm_writel(lnkcap, base + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKCAP);

    lnkctl2 = (lnkctl2 & ~0xf) | PCIE_GEN;
    bcm_writew(lnkctl2, base + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKCTL2);

    wr_fld_rb(base + PCIE_RGR1_SW_INIT_1, PCIE_RGR1_SW_INIT_1_PERST_MASK,
              PCIE_RGR1_SW_INIT_1_PERST_SHIFT, 0);
    k__usleep(100000);

    os_uint32 limit = 100;
    os_uint32 j, i;
    for (i = 1, j = 0; j < limit && !pcie__link_up(base);
         j += i, i = i * 2)
        k__usleep((i + j > limit ? limit - j : i) * 1000);

    if (!pcie__link_up(base))
    {
        k__printf("\nLink down!!!\n");
        return -1;
    }

    if (!pcie__rc_mode(base))
    {
        k__printf("PCIe misconfigured; is in EP mode");
        return -1;
    }

    pcie_set_outbound_win(base, 0, Win__cpu_addr, Win__pcie_addr, Win__size);

    WR_FLD_RB(base, PCIE_RC_CFG_PRIV1_ID_VAL3, CLASS_CODE, 0x060400);

    os_uint16 lnksta = bcm_readw(base + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKSTA);
    os_uint16 cls = lnksta & PCI_EXP_LNKSTA_CLS;
    os_uint16 nlw = (lnksta & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;

    k__printf("Link up, %x Gbps x%x\n", cls, nlw);

    WR_FLD_RB(base, PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1, ENDIAN_MODE_BAR2, DATA_ENDIAN);

    WR_FLD_RB(base, PCIE_MISC_HARD_PCIE_HARD_DEBUG, CLKREQ_DEBUG_ENABLE, 1);

    k__printf("PCIE_MISC_REVISION  %x %x\n", rev, tmp);

    /* https://github.com/rsta2/circle/blob/1884ccf7f630debded2f225f6f931668a6e3264a/lib/bcmpciehostbridge.cpp#L691
     */

    rev = in32(base + 0x8) >> 8; /* revision */
    ht = in8(base + 0xe);        /* header type */
    if (rev != 0x060400 || ht != PCI_HEADER_TYPE_PCI_TO_PCI_BRIDGE)
    {
        k__printf("PCI_HEADER_TYPE_PCI_TO_PCI_BRIDGE failure %x %x\n", rev, ht);
        while (1)
        {
            ;
        }
        return -1;
    }

    out8(base + 0xc, 64 / 4); /*cache line size*/
    out8(base + 0x19, 1);     /*secondary bus*/
    out8(base + 0x1a, 1);     /*subordinate bus*/

    out16(base + 0x20, MEM_PCIE_RANGE_PCIE_START >> 16); /*memory base*/
    out16(base + 0x22, MEM_PCIE_RANGE_PCIE_START >> 16); /*memory limit*/
    out8(base + 0x3e, PCI_BRIDGE_CTL_PARITY);            /*bridge control*/

    cp = in8(base + BRCM_PCIE_CAP_REGS + 0);
    if (cp != PCI_CAPABILITY_PCI_EXPRESS)
    {
        k__printf("PCI_CAPABILITY_PCI_EXPRESS failure %x\n", cp);
        return -1;
    }

    out8(base + BRCM_PCIE_CAP_REGS + PCI_EXP_RTCTL, PCI_EXP_RTCTL_CRSSVE);

    out16(base + 0x4, PCI_COMMAND_MEMORY_SPACE | PCI_COMMAND_BUS_MASTER | PCI_COMMAND_PARITY_ERROR_RESPONSE | PCI_COMMAND_SERR_ENABLE); /*command*/
    return 0;
}

typedef struct PropertyTag
{
    os_uint32 tag_id;
    os_uint32 value_buf_size;
    os_uint32 value_length;
} PropertyTag;

typedef struct PropertyTagSimple
{
    PropertyTag tag;
    os_uint32 value;
} PropertyTagSimple;

typedef struct PropertyBuffer
{
    os_uint32 buf_size;
    os_uint32 code;
    os_uint8 tags[0];
} PropertyBuffer;

#define CODE_RESPONSE_SUCCESS 0x80000000
#define VALUE_LENGTH_RESPONSE (1 << 31)
#define PROPTAG_NOTIFY_XHCI_RESET 0x00030058
#define PROPTAG_END 0x00000000
#define CODE_REQUEST 0x00000000
#define MAILBOX_BASE (PERIPHERAL_BASE + 0xB880)
#define MAILBOX0_READ (MAILBOX_BASE + 0x00)
#define MAILBOX0_STATUS (MAILBOX_BASE + 0x18)
#define MAILBOX1_WRITE (MAILBOX_BASE + 0x20)
#define MAILBOX1_STATUS (MAILBOX_BASE + 0x38)
#define MAILBOX_STATUS_EMPTY 0x40000000
#define MAILBOX_STATUS_FULL 0x80000000
#define BCM_MAILBOX_PROP_OUT 8

os_intn tags__get_tags(void *tag, os_uint32 size)
{
    os_uint32 *end;
    os_uint32 addr;
    os_uint32 data;
    os_uint32 channel = BCM_MAILBOX_PROP_OUT;
    PropertyBuffer *buf = (void *)MEM_COHERENT_REGION;

    buf->buf_size = sizeof(PropertyBuffer) + size + sizeof(os_uint32);
    buf->code = CODE_REQUEST;
    k__memcpy(buf->tags, tag, size);
    end = (os_uint32 *)(buf->tags + size);
    end[0] = PROPTAG_END;

    os__data_sync_barrier();

    addr = BUS_ADDRESS(buf);

    data = addr;

    /*flush*/
    while (!(in32(MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY))
    {
        in32(MAILBOX0_READ);

        k__usleep(20000);
    }

    /*write*/
    while (in32(MAILBOX1_STATUS) & MAILBOX_STATUS_FULL)
    {
        ;
    }
    out32(MAILBOX1_WRITE, channel | data);

    /*read*/
    do
    {
        while (in32(MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY)
        {
            ;
        }

        data = in32(MAILBOX0_READ);
    } while ((data & 0xF) != channel);
    data &= ~0xF;

    if (data != addr)
    {
        k__printf("Mailbox failed\n");
        return -1;
    }

    os__data_mem_barrier();

    if (buf->code != CODE_RESPONSE_SUCCESS)
    {
        k__printf("CODE_RESPONSE_SUCCESS failed\n");
        return -1;
    }

    k__memcpy(tag, buf->tags, size);

    return 0;
}

/* https://github.com/rsta2/circle/blob/master/lib/bcmpciehostbridge.cpp#L722 */
os_intn pci__enable_device(os_uint32 class_code, os_uint32 bus, os_uint32 slot, os_uint32 function)
{
    os_uint8 int_pin;
    os_uint32 rev;
    os_uint8 ht;
    os_intn ret;
    PropertyTagSimple notify_reset;

    notify_reset.value = bus << 20 | slot << 15 | function << 12;
    notify_reset.tag.tag_id = PROPTAG_NOTIFY_XHCI_RESET;
    notify_reset.tag.value_buf_size = sizeof(notify_reset) - sizeof(PropertyTag);
    notify_reset.tag.value_length = 4 & ~VALUE_LENGTH_RESPONSE;
    ret = tags__get_tags(&notify_reset, sizeof(notify_reset));
    notify_reset.tag.value_length &= ~VALUE_LENGTH_RESPONSE;
    if (ret || notify_reset.tag.value_length == 0)
    {
        k__printf("tag failed\n");
    }

    rtos__thread_sleep(20);
    rev = pci__cfg_read_revision(bus, slot, function) >> 8;
    ht = pci__cfg_read_header_type(bus, slot, function);
    if (rev != class_code || ht != PCI_HEADER_TYPE_SINGLE_FUNCTION)
    {
        k__printf("Error pci enable %x %x\n", rev, ht);
        return -1;
    }

    pci__cfg_write_cache_line_size(bus, slot, function, 64 / 4);
    pci__cfg_write_base_addr(bus, slot, function, 0,
                             XHCI_DEFAULT_BASE0 | PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE);
    pci__cfg_write_base_addr(bus, slot, function, 1, 0);

    int_pin = pci__cfg_read_interrupt_pin(bus, slot, function);
    if (int_pin != 1)
    {
        pci__cfg_write_interrupt_pin(bus, slot, function, 1);
    }
    pci__cfg_write_command(bus, slot, function, PCI_COMMAND_MEMORY_SPACE | PCI_COMMAND_BUS_MASTER | PCI_COMMAND_PARITY_ERROR_RESPONSE | PCI_COMMAND_SERR_ENABLE);
    k__printf("\nPci enabled %d %d %d\n", bus, slot, function);

    return 0;
}
