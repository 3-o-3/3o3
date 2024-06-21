

/*
https://github.com/fysnet/FYSOS/blob/master/main/usb/utils/gdevdesc/gd_xhci.c
https://github.com/rsta2/circle/tree/master/lib/usb
*/

#include "../include/xhci.h"

#include "../include/usb.h"

struct xhci
{
    os_uint32 bus;
    os_uint32 slot;
    os_uint32 function;
    os_uint32 irq;
    os_uint32 base0;
    os_uint32 base1;
    os_uint32 size;
};

static struct xhci ctrl[XHCI_MAX_CONTROLLER];

os_intn xhci__find_device(os_intn idx, os_uint32 *bus, os_uint32 *slot, os_uint32 *function)
{
    os_uint32 vendor;
    os_uint32 device;
    os_uint32 class_, subc, pif;
    os_intn i = 0;
    (void)device;
    for (*bus = 0; *bus < PCI_MAX_BUS; (*bus)++)
    {
        for (*slot = 0; *slot < PCI_MAX_SLOT; (*slot)++)
        {
            for (*function = 0; *function < PCI_MAX_FUNCTION; (*function)++)
            {
                vendor = pci__cfg_read_vendor(*bus, *slot, *function);
                if (vendor != PCI_VENDOR_NO_DEVICE)
                {
                    device = pci__cfg_read_device(*bus, *slot, *function);
                    class_ = pci__cfg_read_class(*bus, *slot, *function);
                    subc = pci__cfg_read_subclass(*bus, *slot, *function);
                    pif = pci__cfg_read_prog_if(*bus, *slot, *function);

                    if (class_ == PCI_CLASS_SERIAL_BUS_CONTROLLER && subc == PCI_SUBCLASS_USB && pif == PCI_PROG_IF_XHCI_CONTROLLER)
                    {
                        if (i == idx)
                        {
                            return 0;
                        }
                        i++;
                    }
                }
            }
        }
    }
    return (1);
}

/*
https://github.com/rsta2/circle/blob/master/lib/usb/xhcidevice.cpp
*/
void xhci__init_ctrl(struct xhci *ctrl)
{
    os_intn status;
    os_uint32 v;
    os_uint32 b = XHCI_DEFAULT_BASE0;
    if (ctrl->base0 && !ctrl->base1)
    {
        /*b = ctrl->base0;*/
    }

    status = pci__enable_device(XHCI_PCI_CLASS_CODE, ctrl->bus, ctrl->slot, ctrl->function);
    if (status)
    {
        k__printf("ERROR XHCI.\n");
        return;
    }
    /* b = pci__cfg_read_base_addr(ctrl->bus, ctrl->slot, ctrl->function, 0) & ~0xF;*/
    v = K__r16(b + XHCI_CAPS_IVersion - 2);
    k__printf("%x XHCI Version %x / %x \n", XHCI_SUPPORTED_VERSION, v, b);
}

void xhci__init()
{
    os_intn r;
    os_intn i;
    for (i = 0; i < XHCI_MAX_CONTROLLER; i++)
    {
        r = xhci__find_device(i, &ctrl[i].bus, &ctrl[i].slot, &ctrl[i].function);
        if (r == 0)
        {
            ctrl[i].base0 = pci__cfg_read_base_addr(ctrl[i].bus, ctrl[i].slot, ctrl[i].function, 0) & ~0xF;
            ctrl[i].base1 = pci__cfg_read_base_addr(ctrl[i].bus, ctrl[i].slot, ctrl[i].function, 1);

            ctrl[i].irq = pci__cfg_read_interrupt_line(ctrl[i].bus, ctrl[i].slot, ctrl[i].function);
            ctrl[i].size = pci__mem_range(ctrl[i].bus, ctrl[i].slot, ctrl[i].function, 0x10);
            k__printf("XHCI found: bus %d slot %d func %d base 0x%x:%x irq %d size 0x%x\n", ctrl[i].bus, ctrl[i].slot, ctrl[i].function, ctrl[i].base1, ctrl[i].base0, ctrl[i].irq, ctrl[i].size);
            xhci__init_ctrl(ctrl + i);
        }
    }
    k__printf("done\n");
}
