
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

os_intn pci__init()
{
    return 0;
}

void pci__set_addr(os_uint32 bus, os_uint32 slot,
    os_uint32 function, os_uint32 offset)
{
    os_size address;

    /* Address bits (inclusive):
     * 31      Enable bit (must be 1 for it to work)
     * 30 - 24 Reserved
     * 23 - 16 Bus number
     * 15 - 11 Slot number
     * 10 - 8  Function number (for multifunction devices)
     * 7 - 2   Register number (offset / 4)
     * 1 - 0   Must always be 00 */
    address = 0x80000000 | ((os_size)(bus & 0xff) << 16)
        | ((os_size)(slot & 0x1f) << 11)
        | ((os_size)(function & 0x7) << 8)
        | ((os_size)offset & 0xfc);
    /* Full DWORD write to port must be used for PCI to detect new address. */
    out32(PCI_CONFIG_ADDRESS, address);
}

os_uint8 pci__cfg_read8(os_uint32 bus, os_uint32 slot,
    os_uint32 function, os_uint32 offset)
{
    pci__set_addr(bus, slot, function, offset);
    /* The PCI registers are little endian,
     * so the last byte of DWORD is read
     * when offset is 0. */
    return (in8(PCI_CONFIG_DATA + offset % 4));
}

os_uint16 pci__cfg_read16(os_uint32 bus, os_uint32 slot,
    os_uint32 function, os_uint32 offset)
{
    pci__set_addr(bus, slot, function, offset);
    /* The PCI registers are little endian,
     * so the last word of DWORD is read
     * when offset is 0. */
    return (in16(PCI_CONFIG_DATA + offset % 4));
}

os_uint32 pci__cfg_read32(os_uint32 bus, os_uint32 slot,
    os_uint32 function, os_uint32 offset)
{
    pci__set_addr(bus, slot, function, offset);
    return (in32(PCI_CONFIG_DATA));
}

os_uint8 pci__cfg_write8(os_uint32 bus, os_uint32 slot,
    os_uint32 function, os_uint32 offset,
    os_uint8 data)
{
    pci__set_addr(bus, slot, function, offset);
    /* The PCI registers are little endian,
     * so the last byte of DWORD is written
     * when offset is 0. */
    out8(PCI_CONFIG_DATA + offset % 4, data);
    return (in8(PCI_CONFIG_DATA + offset % 4));
}

os_uint16 pci__cfg_write16(os_uint32 bus, os_uint32 slot,
    os_uint32 function, os_uint32 offset,
    os_uint32 data)
{
    pci__set_addr(bus, slot, function, offset);
    /* The PCI registers are little endian,
     * so the last word of DWORD is written
     * when offset is 0. */
    out16(PCI_CONFIG_DATA + offset % 4, data);
    return (in16(PCI_CONFIG_DATA + offset % 4));
}

os_uint32 pci__cfg_write32(os_uint32 bus, os_uint32 slot,
    os_uint32 function, os_uint32 offset,
    os_size data)
{
    pci__set_addr(bus, slot, function, offset);
    out32(PCI_CONFIG_DATA, data);
    return (in32(PCI_CONFIG_DATA));
}

os_intn pci__find_device(os_uint32 vendor, os_uint32 device,
    os_uint32 index, os_uint32* bus,
    os_uint32* slot, os_uint32* function)
{
    os_uint32 read_vendor;
    os_uint8 read_header;

    for (*bus = 0; *bus < PCI_MAX_BUS; (*bus)++) {
        for (*slot = 0; *slot < PCI_MAX_SLOT; (*slot)++) {
            read_vendor = pci__cfg_read_vendor(*bus, *slot, 0);
            if (read_vendor != PCI_VENDOR_NO_DEVICE) {
                /* In case it is a multifunction device,
                 * all functions must always be checked
                 * as they do not have to be in order. */
                read_header = pci__cfg_read_header_type(*bus, *slot, 0);
                if (read_header & PCI_HEADER_TYPE_MULTI_FUNCTION) {
                    for (*function = 0;
                         *function < PCI_MAX_FUNCTION;
                         (*function)++) {
                        if (pci__cfg_read_vendor(*bus, *slot, *function)
                            == vendor) {
                            if (pci__cfg_read_device(*bus, *slot, *function)
                                == device) {
                                if (index == 0)
                                    return (0);
                                index--;
                            }
                        }
                    }
                }
                /* Otherwise it must be a single function device
                 * and we check only the first function. */
                else if (read_vendor == vendor) {
                    if (pci__cfg_read_device(*bus, *slot, 0) == device) {
                        if (index == 0) {
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

os_intn pci__find_class_code(os_uint8 class, os_uint8 subclass,
    os_uint8 prog_IF, os_uint32 index,
    os_uint32* bus, os_uint32* slot,
    os_uint32* function)
{
    os_uint32 max_function;
    for (*bus = 0; *bus < PCI_MAX_BUS; (*bus)++) {
        for (*slot = 0; *slot < PCI_MAX_SLOT; (*slot)++) {
            if (pci__cfg_read_vendor(*bus, *slot, 0) != PCI_VENDOR_NO_DEVICE) {
                /* In case it is a multifunction device,
                 * all functions must always be checked
                 * as they do not have to be in order. */
                if (pci__cfg_read_header_type(*bus, *slot, 0)
                    & PCI_HEADER_TYPE_MULTI_FUNCTION) {
                    max_function = PCI_MAX_FUNCTION;
                }
                /* Otherwise it must be a single function device
                 * and we check only the first function. */
                else
                    max_function = 1;
                for (*function = 0;
                     *function < max_function;
                     (*function)++) {
                    if (class == PCI_CLASS_WILDCARD || (pci__cfg_read_class(*bus, *slot, *function) == class)) {
                        if (subclass == PCI_SUBCLASS_WILDCARD || (pci__cfg_read_subclass(*bus, *slot, *function) == subclass)) {
                            if (prog_IF == PCI_PROG_IF_WILDCARD || (pci__cfg_read_prog_if(*bus, *slot, *function) == prog_IF)) {
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

/*https://github.com/fysnet/FYSOS/blob/master/main/usb/utils/include/pci.h*/
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
    if ((org0 & 0x07) == PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE) {
        org1 = pci__cfg_read32(bus, dev, func, offset + 4);
    }

    pci__cfg_write32(bus, dev, func, offset, 0xFFFFFFFF);
    if ((org0 & 0x07) == PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE) {
        pci__cfg_write32(bus, dev, func, offset + 4, 0xFFFFFFFF);
    }

    range[0] = pci__cfg_read32(bus, dev, func, offset);
    range[1] = 0;
    if ((org0 & 0x07) == PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE) {
        range[1] = pci__cfg_read32(bus, dev, func, offset + 4);
    }

    pci__cfg_write32(bus, dev, func, offset, org0);
    if ((org0 & 0x07) == PCI_BASE_ADDRESS_MEMORY_64BIT_SPACE) {
        pci__cfg_write32(bus, dev, func, offset + 4, org1);
    }

    pci__cfg_write_command(bus, dev, func, cmnd);

    if (org0 & PCI_BASE_ADDRESS_IO_SPACE) {
        org0 = range[0];
        if ((org0 & 0xFFFF0000) == 0) {
            org0 |= 0xFFFF0000;
        }
        return (os_size)(~(org0 & ~0x1) + 1);
    } else {
        return (~(range[0] & ~0xF) + 1);
    }
}

os_intn pci__enable_device(os_uint32 class_code, os_uint32 bus, os_uint32 slot, os_uint32 function)
{
    return 0;
}