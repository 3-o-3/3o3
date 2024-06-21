
/*
 *                       OS-3o3 Operating System
 *
 *                        AMD64 UEFI boot loader.
 *                  It switches the CPU to 32 bit mode,
 *                    then it calls the 32 bit kernel
 *
 *                      13 may MMXXIV PUBLIC DOMAIN
 *           The authors disclaim copyright to this source code.
 *
 *
 * https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#efi-graphics-output-protocol
 * https://uefi.org/specs/UEFI/2.10/13_Protocols_Media_Access.html
 * https://stackoverflow.com/questions/22962251/how-to-enter-64-bit-mode-on-a-x86-64
 *
 */

#include "../../include/klib.h"
#include "../../include/pci.h"

extern char io32[];
#include "../../bin/io32.h"

#define efi_handle void *
#define efi_status int
#define efi_physical_address void *
#define efi_success 0

typedef struct
{
    os_uint32 Data1;
    os_uint16 Data2;
    os_uint16 Data3;
    os_uint8 Data4[8];
} efi_guid;

#define efi_graphic_output_protocol_guid                   \
    {                                                      \
        0x9042a9de, 0x23dc, 0x4a38,                        \
        {                                                  \
            0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a \
        }                                                  \
    }

#define efi_acpi_20_table_guid                             \
    {                                                      \
        0x8868e871, 0xe4f1, 0x11d3,                        \
        {                                                  \
            0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 \
        }                                                  \
    }

typedef struct efi_graphics_output_protocol efi_graphics_output_protocol;

typedef enum
{
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} efi_graphics_pixel_format;

typedef struct
{
    os_uint32 RedMask;
    os_uint32 GreenMask;
    os_uint32 BlueMask;
    os_uint32 ReservedMask;
} efi_pixel_bitmask;

typedef struct
{
    os_uint32 Version;
    os_uint32 HorizontalResolution;
    os_uint32 VerticalResolution;
    efi_graphics_pixel_format PixelFormat;
    efi_pixel_bitmask PixelInformation;
    os_uint32 PixelsPerScanLine;
} efi_graphics_output_mode_information;

typedef struct
{
    os_uint32 MaxMode;
    os_uint32 Mode;
    efi_graphics_output_mode_information *Info;
    os_intn SizeOfInfo;
    efi_physical_address FrameBufferBase;
    os_intn FrameBufferSize;
} efi_graphics_output_protocol_mode;

typedef efi_status (*efi_graphics_output_protocol_query_mode)(
    efi_graphics_output_protocol *This,
    os_uint32 ModeNumber,
    os_intn *SizeOfInfo,
    efi_graphics_output_mode_information **Info);

typedef efi_status (*efi_graphics_output_protocol_set_mode)(
    efi_graphics_output_protocol *This,
    os_uint32 ModeNumber);

typedef struct
{
    os_uint8 Blue;
    os_uint8 Green;
    os_uint8 Red;
    os_uint8 Reserved;
} efi_graphics_output_blt_pixel;

typedef enum
{
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} efi_graphics_output_blt_operation;

typedef efi_status (*efi_graphics_output_protocol_blt)(
    efi_graphics_output_protocol *This,
    efi_graphics_output_blt_pixel *BltBuffer,
    efi_graphics_output_blt_operation BltOperation,
    os_intn SourceX,
    os_intn SourceY,
    os_intn DestinationX,
    os_intn DestinationY,
    os_intn Width,
    os_intn Height,
    os_intn Delta);

typedef struct efi_graphics_output_protocol
{
    efi_graphics_output_protocol_query_mode QueryMode;
    efi_graphics_output_protocol_set_mode SetMode;
    efi_graphics_output_protocol_blt Blt;
    efi_graphics_output_protocol_mode *Mode;
} efi_graphics_output_protocol;

#define efi_system_table_signature 0x5453595320494249

typedef struct
{
    os_uint64 Signature;
    os_uint32 Revision;
    os_uint32 HeaderSize;
    os_uint32 CRC32;
    os_uint32 Reserved;
} efi_table_header;

typedef efi_status (*efi_locate_protocol)(efi_guid *Protocol,
                                          void *Registration,
                                          void **Interface);

typedef efi_status (*efi_exit_boot_services)(efi_handle ImageHandle,
                                             os_intn MapKey);

#define efi_raise_tpl void *
#define efi_restore_tpl void *
#define efi_allocate_pages void *
#define efi_free_pages void *

#define UEFI_MMAP_SIZE 0x4000
#define efi_memory_descriptor os_uint8
typedef efi_status (*efi_get_memory_map)(os_intn *MemoryMapSize,
                                         efi_memory_descriptor *MemoryMap,
                                         os_intn *MapKey,
                                         os_intn *DescriptorSize,
                                         os_uint32 *DescriptorVersion);

#define efi_allocate_pool void *
#define efi_free_pool void *
#define efi_create_event void *
#define efi_set_timer void *
#define efi_wait_for_event void *
#define efi_signal_event void *
#define efi_close_event void *
#define efi_check_event void *
#define efi_install_protocol_interface void *
#define efi_reinstall_protocol_interface void *
#define efi_uninstall_protocol_interface void *
#define efi_handle_protocol void *
#define efi_register_protocol_notify void *
#define efi_locate_handle void *
#define efi_locate_device_path void *
#define efi_install_configuration_table void *
#define efi_image_load void *
#define efi_image_start void *
#define efi_exit void *
#define efi_image_unload void *
#define efi_get_next_monotonic_count void *
#define efi_stall void *
#define efi_set_watchdog_timer void *
#define efi_connect_controller void *
#define efi_disconnect_controller void *
#define efi_open_protocol void *
#define efi_close_protocol void *
#define efi_open_protocol_information void *
#define efi_protocols_per_handle void *
#define efi_locate_handle_buffer void *
#define efi_install_multiple_protocol_interfaces void *
#define efi_uninstall_multiple_protocol_interfaces void *
#define efi_calculate_crc32 void *
#define efi_copy_mem void *
#define efi_set_mem void *
#define efi_create_event_ex void *

typedef struct
{
    efi_table_header Hdr;

    efi_raise_tpl RaiseTPL;
    efi_restore_tpl RestoreTPL;

    efi_allocate_pages AllocatePages;
    efi_free_pages FreePages;
    efi_get_memory_map GetMemoryMap;
    efi_allocate_pool AllocatePool;
    efi_free_pool FreePool;

    efi_create_event CreateEvent;
    efi_set_timer SetTimer;
    efi_wait_for_event WaitForEvent;
    efi_signal_event SignalEvent;
    efi_close_event CloseEvent;
    efi_check_event CheckEvent;

    efi_install_protocol_interface InstallProtocolInterface;
    efi_reinstall_protocol_interface ReinstallProtocolInterface;
    efi_uninstall_protocol_interface UninstallProtocolInterface;
    efi_handle_protocol HandleProtocol;
    void *Reserved;
    efi_register_protocol_notify RegisterProtocolNotify;
    efi_locate_handle LocateHandle;
    efi_locate_device_path LocateDevicePath;
    efi_install_configuration_table InstallConfigurationTable;

    efi_image_load LoadImage;
    efi_image_start StartImage;
    efi_exit Exit;
    efi_image_unload UnloadImage;
    efi_exit_boot_services ExitBootServices;

    efi_get_next_monotonic_count GetNextMonotonicCount;
    efi_stall Stall;
    efi_set_watchdog_timer SetWatchdogTimer;

    efi_connect_controller ConnectController;
    efi_disconnect_controller DisconnectController;

    efi_open_protocol OpenProtocol;
    efi_close_protocol CloseProtocol;
    efi_open_protocol_information OpenProtocolInformation;

    efi_protocols_per_handle ProtocolsPerHandle;
    efi_locate_handle_buffer LocateHandleBuffer;
    efi_locate_protocol LocateProtocol;
    efi_install_multiple_protocol_interfaces InstallMultipleProtocolInterfaces;
    efi_uninstall_multiple_protocol_interfaces
        UninstallMultipleProtocolInterfaces;

    efi_calculate_crc32 CalculateCrc32;

    efi_copy_mem CopyMem;
    efi_set_mem SetMem;
    efi_create_event_ex CreateEventEx;
} efi_boot_services;

typedef struct _efi_simple_text_output_protocol efi_simple_text_output_protocol;

typedef efi_status (*efi_text_string)(efi_simple_text_output_protocol *This,
                                      os_utf16 *String);

#define efi_text_reset void *
#define efi_text_test_string void *
#define efi_text_query_mode void *
#define efi_text_set_mode void *
#define efi_text_set_attribute void *
#define efi_text_clear_screen void *
#define efi_text_set_cursor_position void *
#define efi_text_enable_cursor void *
#define simple_text_output_mode void *

typedef struct _efi_simple_text_output_protocol
{
    efi_text_reset Reset;
    efi_text_string OutputString;
    efi_text_test_string TestString;
    efi_text_query_mode QueryMode;
    efi_text_set_mode SetMode;
    efi_text_set_attribute SetAttribute;
    efi_text_clear_screen ClearScreen;
    efi_text_set_cursor_position SetCursorPosition;
    efi_text_enable_cursor EnableCursor;
    simple_text_output_mode *Mode;
} efi_simple_text_output_protocol;

#define efi_simple_text_input_protocol void *
#define efi_runtime_services void *
typedef struct
{
    efi_guid VendorGuid;
    void *VendorTable;
} efi_configuration_table;

efi_graphics_output_protocol *gfx;

typedef struct
{
    efi_table_header Hdr;
    os_utf16 *FirmwareVendor;
    os_uint32 FirmwareRevision;
    efi_handle ConsoleInHandle;
    efi_simple_text_input_protocol *ConIn;
    efi_handle ConsoleOutHandle;
    efi_simple_text_output_protocol *ConOut;
    efi_handle StandardErrorHandle;
    efi_simple_text_output_protocol *StdErr;
    efi_runtime_services *RuntimeServices;
    efi_boot_services *BootServices;
    os_intn NumberOfTableEntries;
    efi_configuration_table *ConfigurationTable;
} efi_system_table;

void *ImageHandle;
efi_system_table *SystemTable;
efi_simple_text_output_protocol *ConOut;
efi_text_string echo;
static os_uint8 *gfx_info;
static os_uint8 MemoryMap[UEFI_MMAP_SIZE];
volatile os_uint8 *k__fb = 0;
os_uint16 k__fb_height;
os_uint16 k__fb_width;
os_uint16 k__fb_pitch;
os_uint16 k__fb_bpp;

/* set frambuffer properties */
static void set_fb_address(os_uint32 *fb)
{
    *((os_uint32 **)(&gfx_info[40])) = (os_uint32 *)fb;
    k__fb = (os_uint8 *)fb;
}

void k__fb_init()
{
    k__fb = 0;
}

static void set_fb_linear()
{
    gfx_info[0] = 0x08;
}

static void set_fb_width(os_uint16 w)
{
    *((os_uint16 *)(&gfx_info[18])) = w;
    k__fb_width = w;
}

static void set_fb_height(os_uint16 h)
{
    *((os_uint16 *)(&gfx_info[20])) = h;
    k__fb_height = h;
}

static void set_fb_pitch(os_uint16 p)
{
    *((os_uint16 *)(&gfx_info[16])) = p;
    k__fb_pitch = p;
}

static void set_fb_bpp(os_uint8 b)
{
    gfx_info[25] = b;
    k__fb_bpp = b;
}

static void set_fb_rgb(os_uint8 b)
{
    gfx_info[30] = b;
}

/* GDT */
static void gdt_entry(os_uint32 offset,
                      os_uint16 limit15_0,
                      os_uint16 base15_0,
                      os_uint8 base23_16,
                      os_uint8 type,
                      os_uint8 limit19_16_and_flags,
                      os_uint8 base31_24)
{
    os_uint8 *g = (os_uint8 *)((void *)GDT_ADDR + offset);
    (*(os_uint16 *)&g[0]) = limit15_0;
    (*(os_uint16 *)&g[2]) = base15_0;
    g[4] = base23_16;
    g[5] = type;
    g[6] = limit19_16_and_flags;
    g[7] = base31_24;
}

/* GDTR */
static void gdtr()
{
    os_uint16 *g = (os_uint16 *)(GDTR_ADDR);
    g[0] = GDTR_ADDR - GDT_ADDR - 1;
    *((os_uint64 *)(g + 1)) = GDT_ADDR;
}

/**
 * write string to UEFI text console
 */
void uefi__puts(char *str)
{
    os_uint16 buf[256];
    os_intn i = 0;
    os_intn j = 0;
    while (str[i] && i < 254)
    {
        if (str[i] == '\n')
        {
            buf[j] = '\r';
            j++;
        }
        buf[j] = (os_uint8)str[i];
        i++;
        j++;
    }
    buf[j] = 0;
    echo(ConOut, buf);
}

/**
 * remap a page using the MMU
 */
os_uint32 remap(os_uint32 *page_directory, os_uint32 default_addr, os_uint64 src, os_uint32 len)
{
    os_intn i;
    len = len / 0x400000;
    if (len < 1)
    {
        len = 1;
    }
    for (i = default_addr / 0x400000; i < (default_addr / 0x400000 + len);
         i++)
    {
        os_uint32 bit39_32 = src >> 32; /* high address bits*/
        os_uint32 a = i - (default_addr / 0x400000) + ((src & 0xFFFFFFFF) / 0x400000);
        page_directory[i] = (bit39_32 << 13) | ((a) << 22) | 0x83; /* P RW US PS */
    }
    k__printf(" Remapped %x:%x -> %x len: %x\r\n", (src >> 32) & 0xFFFFFFFF,
              src & 0xFFFFFFFF, default_addr, len * 0x400000);
    return len;
}

/**
 * re-map XHCI address range if needed
 */
void remap_xhci_ctrl(os_uint32 *page_directory, os_intn num, os_uint32 low, os_uint32 high, os_uint32 size)
{
    os_uint32 da = XHCI_DEFAULT_BASE0;
    if (high == 0)
    {
        return;
    }
    if (num == 1)
    {
        da = XHCI_DEFAULT_BASE1;
    }
    remap(page_directory, da, (((os_uint64)high) << 32) + low, size);
}

/**
 * remap the PCI XHCI address using MMU pageing
 */
os_intn remap_xhci(os_uint32 *page_directory)
{
    os_uint32 b = 0;
    os_uint32 s = 0;
    os_uint32 f = 0;
    os_uint32 vendor;
    os_uint32 class_, subc, pif;
    os_intn i = 0;
    os_uint32 *bus = &b;
    os_uint32 *slot = &s;
    os_uint32 *function = &f;
    os_uint32 high, low, size;

    for (*bus = 0; *bus < PCI_MAX_BUS; (*bus)++)
    {
        for (*slot = 0; *slot < PCI_MAX_SLOT; (*slot)++)
        {
            for (*function = 0; *function < PCI_MAX_FUNCTION; (*function)++)
            {
                vendor = pci__cfg_read_vendor(*bus, *slot, *function);
                if (vendor != PCI_VENDOR_NO_DEVICE)
                {
                    /*device = pci__cfg_read_device(*bus, *slot, *function);*/
                    class_ = pci__cfg_read_class(*bus, *slot, *function);
                    subc = pci__cfg_read_subclass(*bus, *slot, *function);
                    pif = pci__cfg_read_prog_if(*bus, *slot, *function);

                    if (class_ == PCI_CLASS_SERIAL_BUS_CONTROLLER && subc == PCI_SUBCLASS_USB && pif == PCI_PROG_IF_XHCI_CONTROLLER)
                    {
                        if (i >= XHCI_MAX_CONTROLLER)
                        {
                            k__printf("Too many xHCI controller found\n");
                        }
                        else
                        {
                            low = pci__cfg_read_base_addr(*bus, *slot, *function, 0);
                            high = pci__cfg_read_base_addr(*bus, *slot, *function, 1);
                            size = pci__cfg_write_base_addr(*bus, *slot, *function, 0, 0xFFFFFFFF) & ~0xF;
                            pci__cfg_write_base_addr(*bus, *slot, *function, 0, low);
                            size = (~size) + 1;
                            remap_xhci_ctrl(page_directory, i, low & ~0x0F, high, size);
                        }
                        i++;
                    }
                }
            }
        }
    }
    return 0;
}

/**
 * UEFI main loader function
 */
int main(void *ih, efi_system_table *st)
{
    os_intn MemoryMapSize = UEFI_MMAP_SIZE;
    os_intn MapKey, DescriptorSize;
    os_uint32 DescriptorVersion;
    efi_graphics_output_mode_information *info = (efi_graphics_output_mode_information *)(void *)0;
    os_intn size;
    os_uint32 i;
    os_uint32 mode = 0xFFFFF;
    efi_guid guid = efi_graphic_output_protocol_guid;
    efi_status r;
    os_uint64 fbaddr;
    efi_configuration_table *ct;
    efi_guid ac = efi_acpi_20_table_guid;
    os_uint32 *page_directory;

    ImageHandle = ih;
    SystemTable = st;
    gfx_info = (os_uint8 *)GFX_INFO;
    echo = st->ConOut->OutputString;
    ConOut = st->ConOut;
    k__printf(" OS-3o3 Image: %x", ImageHandle);

    SystemTable->BootServices->LocateProtocol(&guid, (void *)0, (void **)&gfx);
    k__printf(" GFX: %x\r\n", gfx);

    /* copy the 32bit kernel to is location */
    os_uint64 *dst = (os_uint64 *)(KERNEL_ENTRY32);
    i = 0;
    while (i < sizeof(io32))
    {
        *dst = *((os_uint64 *)(&io32[i]));
        dst++;
        i += 8;
    }

    /* ACPI parsing */
    ct = st->ConfigurationTable;
    i = st->NumberOfTableEntries;
    while (i > 0)
    {
        if (!k__memcmp(&ct->VendorGuid, &ac, sizeof(efi_guid)))
        {
            acpi__init(ct->VendorTable);
        }
        ct++;
        i--;
    }

    /* try to find graphic framebuffer 640x480 32bpp */
    i = 0;
    while (!gfx->QueryMode(gfx, i, &size, &info))
    {
        if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor || info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
        {
            k__printf("%d x %d ", info->HorizontalResolution,
                      info->VerticalResolution);
            switch (info->PixelFormat)
            {
            case PixelRedGreenBlueReserved8BitPerColor: /* byte[0] = Red */
                k__printf(" RGBA");
                break;
            case PixelBlueGreenRedReserved8BitPerColor: /* byte[0] = Blue */
                k__printf(" BGRA");
                break;
            default:
                k__printf("%x", (long long)info->PixelFormat);
            }
            if (info->PixelsPerScanLine != 640)
            {
                k__printf(" pxl/scan %d", (long long)info->PixelsPerScanLine);
            }
            k__printf("\r\n");
            if (mode == 0xFFFFF)
            {
                mode = i;
            }
            else if (info->HorizontalResolution == 640 && info->VerticalResolution == 480)
            {
                mode = i;
                break;
            }
        }
        i++;
    }

    if (mode == 0xFFFFF)
    {
        k__printf("No compatible graphic mode found\r\n");
        while (1)
        {
            ;
        }
    }
    else
    {
        /* switch to GOP */
        if (gfx->SetMode(gfx, mode))
        {
            k__printf("Error switching graphic mode\r\n");
            while (1)
            {
                ;
            }
        }
    }

    fbaddr = (os_uint64)gfx->Mode->FrameBufferBase;
    set_fb_linear();
    set_fb_width(gfx->Mode->Info->HorizontalResolution);
    set_fb_height(gfx->Mode->Info->VerticalResolution);
    set_fb_bpp(32);
    set_fb_pitch(gfx->Mode->Info->PixelsPerScanLine * 4);
    set_fb_rgb(info->PixelFormat);

    /*https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
      4.3 32-BIT PAGING sdm-vol-3abcd.pdf
    */
    page_directory = (os_uint32 *)(PDT_ADDR);
    for (i = 0; i < 1024; i++)
    {
        page_directory[i] = ((i) << 22) | 0x83;
    }
    set_fb_address((os_uint32 *)(fbaddr & 0xFFFFFFFF));
    if (fbaddr > 0xFFFFFFFFULL)
    {
        set_fb_address((os_uint32 *)FB_DEFAULT_ADDR);
        remap(page_directory, FB_DEFAULT_ADDR, fbaddr, 64 * 0x400000);
    }
    k__printf(" GFX: %x", (long long)gfx->Mode->Mode);
    k__printf("%x", (long long)mode);
    k__printf(" Frame buffer: ");
    k__printf("%x", (long long)gfx->Mode->FrameBufferBase);
    k__printf(" --- ");
    k__printf("%x", (long long)gfx->Mode->FrameBufferSize);

    remap_xhci(page_directory);

    /* Quit UEFI services */
    r = efi_success;
    st->BootServices->GetMemoryMap(
        &MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    r = st->BootServices->ExitBootServices(ImageHandle, MapKey);
    if (r != efi_success)
    {
        k__printf("\r\n Panic UEFI failure code ");
        k__printf("%x", r);
        k__printf("\r\n");
        while (1)
        {
            ;
        }
    }

    /*  https://wiki.osdev.org/GDT_Tutorial
      https://blog.llandsmeer.com/tech/2019/07/21/uefi-x64-userland.html */
    gdt_entry(0x00, 0, 0, 0, 0x00, 0x00, 0); /* null */
    gdt_entry(0x08,
              0xFFFF,
              0,
              0,
              0x9A,
              0xCF,
              0);                                 /* 32 bit compatibility mode code segment */
    gdt_entry(0x10, 0xFFFF, 0, 0, 0x92, 0xCF, 0); /* 32 bit data segment */
    gdt_entry(0x18, 0xFFFF, 0, 0, 0x9A, 0xAF, 0); /* code segment */
    gdt_entry(0x20, 0xFFFF, 0, 0, 0x92, 0xAF, 0); /* data segment */
    gdtr();

    /* jump to 32bit mode */
    load_gdt();

    /* should never been here */
    while (1)
    {
        ;
    }
    return -1;
}
