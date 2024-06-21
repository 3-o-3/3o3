/*
 *                        OS-3o3 operating system
 *
 *               Advanced Configuration and Power Interface
 *
 *                      13 may MMXXIV PUBLIC DOMAIN
 *
 *           The authors disclaim copyright to this source code.
 *
 *
 * https://github.com/pdoane/osdev/blob/master/acpi/acpi.c
 */

#include "../../include/arch/i386/defs.h"
#include "../../include/klib.h"

typedef struct AcpiHeader
{
    os_uint32 signature;
    os_uint32 length;
    os_uint8 data[8]; /* revision, checksum, oem[6] */
    os_uint8 oem_table_id[8];
    os_uint32 oem_revision;
    os_uint32 creator_id;
    os_uint32 creator_revision;
} AcpiHeader;

/* compare 4 byte signature */
static os_intn sigcmp(os_uint8 *s, os_utf8 *sig)
{
    return ((os_uint32 *)s)[0] - ((os_uint32 *)sig)[0];
}

/* compute check sum */
static os_intn check_csum(os_void *d, os_intn l)
{
    os_uint8 sum = 0;
    os_uint8 *p = (os_uint8 *)d;
    os_intn i;
    for (i = 0; i < l; i++)
    {
        sum += p[i];
    }
    return sum;
}

/* processor local APIC */
static void apic_entry0(os_uint8 *p)
{
    os_uint8 processor_id = *(p + 0x2);
    os_uint8 apic_id = *(p + 0x3);
    os_uint32 flags = *((os_uint32 *)(p + 0x4));
    k__printf("%x", processor_id); /* 1 */
    k__printf("%x", apic_id);      /* 0 */
    k__printf("%x", flags);        /* 1 */
    k__printf(" 0\n");
}

/* I / O APIC */
static void apic_entry1(os_uint8 *p)
{
    os_uint8 io_apic_id = *(p + 0x2);
    os_uint32 io_apic_address = *((os_uint32 *)(p + 0x4));
    os_uint32 global_sys_int_base = *((os_uint32 *)(p + 0x8));
    k__printf("%x", io_apic_id);          /* 2 */
    k__printf("%x", io_apic_address);     /* 0xFEC00000 */
    k__printf("%x", global_sys_int_base); /* 0 */
    k__printf(" 1\n");
}
/* I / O APIC Interrupt source override */
static void apic_entry2(os_uint8 *p)
{
    os_uint8 bus_source = *(p + 0x2);
    os_uint8 irq_source = *(p + 0x3);
    os_uint32 global_interrupt = *((os_uint32 *)(p + 0x4));
    os_uint16 flags = *((os_uint16 *)(p + 0x8));
    k__printf("%x", bus_source);       /* 0 0 */
    k__printf("%x", irq_source);       /* 0 9 */
    k__printf("%x", global_interrupt); /* 2 9 */
    k__printf("%x", flags);            /* 0 D */
    k__printf(" 2\n");
}

/* Local APIC Non - Maskable interrupts */
static void apic_entry4(os_uint8 *p)
{
    os_uint8 acpi_cpu_id = *(p + 0x2); /* 0xFF = all cpu */
    os_uint16 flags = *((os_uint16 *)(p + 0x3));
    os_uint8 lint = *(p + 0x5);
    k__printf("%x", acpi_cpu_id); /* 1 */
    k__printf("%x", flags);       /* 5 */
    k__printf("%x", lint);        /* 1 */
    k__printf(" 4\n");
}

/* https://wiki.osdev.org/MADT */
static void acpi_parse_madt(AcpiHeader *dt)
{
    os_uint8 *p = (os_uint8 *)dt;
    os_uint8 *end = p + dt->length;
    os_uint32 local_apic_address = *((os_uint32 *)(p + 0x24));
    /*os_uint32 flags = *((os_uint32 *)(p + 0x28));*/
    os_uint8 *rec = p + 0x2C;
    os_uint8 et;
    os_uint8 rl;
    k__printf("%x", local_apic_address);
    k__printf(" -> APIC base\n");

    while (rec < end)
    {
        et = rec[0];
        rl = rec[1];
        switch (et)
        {
        case 0:
            apic_entry0(rec);
            break;
        case 1:
            apic_entry1(rec);
            break;
        case 2:
            apic_entry2(rec);
            break;
        case 4:
            apic_entry4(rec);
            break;
        default:
            k__printf("\nUnknonw APIC record: ");
            k__printf("%x", et);
        }
        rec += rl;
    }
}

/* High precistion timer (currently unused)*/
static void acpi_parse_hpet(AcpiHeader *dt)
{
    os_uint8 *p = (os_uint8 *)dt;
    os_uint64 base_address = *((os_uint64 *)(p + 44));
    /*os_uint32 event_timer_block_id = *((os_uint32 *)(p + 36));
    os_uint8 address_space_id = p[40];
    os_uint8 reg_bit_width = p[41];
    os_uint8 reg_bit_offset = p[42];
    os_uint8 hpet_number = p[52];
    os_uint16 main_counter_min_tick = *((os_uint16 *)(p + 53));
    os_uint8 page_protection = p[55];
    */
    os_uint32 *reg;
    os_uint32 v;

    reg = (os_uint32 *)(base_address + 0x0); /* capability register */
    v = reg[0];
    reg = (os_uint32 *)(base_address + 0x10); /* general config register */
    if (reg[0] != 0)
    {
        if (v & 0x8000)
        {
            *((os_uint32 *)HPET_CONFIG) = base_address + 0x10;
            k__printf("%x", base_address);
            k__printf(" Legacy routing supported.\n");
            reg = (os_uint32 *)(base_address + 0x10); /* general config register */
            reg[0] = 0;                               /* disable */

            reg = (os_uint32 *)(base_address + 0xF0); /* main counter */
            reg[0] = 0;                               /* clear */

            reg = (os_uint32 *)(base_address + 0x100); /* timer 0 configuration */
            reg[0] &= ~((1 << 1));                     /* Tn_INT_TYPE_CNF */
            reg[0] |= (1 << 3) | (1 << 2) | 0x40;      /* Tn_TYPE_CNF | Tn_INT_ENB_CNF */

            reg = (os_uint32 *)(base_address + 0x108); /*  TIMER0_COMPARATOR_VAL */
            reg[0] = 0xFFF;
            reg[1] = 0x0;

            reg = (os_uint32 *)(base_address + 0x10); /* general config register */
            /* reg[0] = 3;*/                          /* LEG_RT_CNF | ENABLE_CNF */
        }
        else
        {
            k__printf("disabling HPET!\n");
        }
    }
}

static void acpi_parse_dt(AcpiHeader *dt)
{
    char oem[10];
    char sig[5];
    os_uint32 s;

    k__memcpy(sig, dt, 4);
    sig[4] = 0;
    k__memcpy(oem, dt->data + 2, 6);
    oem[6] = 0;
    if (0)
    {
        k__printf(sig);
        k__printf(" / ");
        k__printf(oem);
        k__printf("\n");
    }
    if (check_csum(dt, dt->length) != 0)
    {
        k__printf(" - Error in table checksum !!\n");
        return;
    }
    s = (dt->signature & 0xFF) << 24;
    s |= (dt->signature & 0xFF00) << 8;
    s |= (dt->signature & 0xFF0000) >> 8;
    s |= (dt->signature & 0xFF000000) >> 24;

    switch (s)
    {
    case 'FACP': /* Fixed ACPI Description Table(FADT) */
        break;
    case 'APIC': /* Multiple APIC Description Table */
        acpi_parse_madt(dt);
        break;
    case 'HPET': /* IA-PC High Precision Event Timer Table */
        acpi_parse_hpet(dt);
        break;
    case 'WAET': /* Windows ACPI Emulated Devices Table */
        break;
    case 'BGRT': /* Boot Graphics Resource Table */
        break;
    }
}

/* 5.2.7 Root System Description Table(RSDT) */
static void acpi_parse_rsdt(AcpiHeader *rsdt)
{
    os_uint32 *p;
    os_uint32 *end;
    char oem[10];
    p = (os_uint32 *)(rsdt + 1);
    k__memcpy(oem, rsdt->data + 2, 6);
    oem[6] = 0;
    k__printf("\nRSDT\n");
    k__printf("%x", rsdt->length);
    k__printf(oem);
    k__printf("\n");
    if (check_csum(rsdt, rsdt->length) != 0)
    {
        k__printf("Error in RSDT checksum\n");
        return;
    }
    end = (os_uint32 *)(((os_uint8 *)rsdt) + rsdt->length);
    while (p < end)
    {
        acpi_parse_dt((void *)(os_intn)(*((os_uint32 *)p) & 0xFFFFFFFF));
        p++;
    }
}

/* 5.2.8 Extended System Description Table(XSDT) */
static void acpi_parse_xsdt(AcpiHeader *xsdt)
{
    os_uint64 *p;
    os_uint64 *end;
    os_utf8 oem[10];
    p = (os_uint64 *)(xsdt + 1);
    k__memcpy(oem, xsdt->data + 2, 6);
    oem[6] = 0;
    k__printf("\nXSDT\n");
    k__printf("%x", xsdt->length);
    k__printf(" ");
    k__printf(oem);
    k__printf("\n");
    if (check_csum(xsdt, xsdt->length) != 0)
    {
        k__printf("Error in XSDT checksum\n");
        return;
    }
    end = (os_uint64*)(((os_uint8 *)xsdt) + xsdt->length);
    while (p < end)
    {
        acpi_parse_dt((void *)*((os_uint64 *)p));
        p++;
    }
}

/* ACPI spec 6.5 / 5.2.5.3 Root System Description Pointer(RSDP) Structure */
static os_intn acpi_parse_rsdp(os_uint8 *p)
{
    char oem[7];
    if (check_csum(p, 20) != 0)
    {
        k__printf("RSDP checksum failed\n");
        return -1;
    }
    k__memcpy(oem, p + 9, 6); /* OEMID */
    oem[6] = 0;
    k__printf("OEM = ");
    k__printf(oem);

    if (p[15] == 0)
    { /* Revision */
        k__printf("\nVersion 1\n");
        acpi_parse_rsdt((void *)(os_intn)(*((os_uint32 *)(p + 16)) & 0xFFFFFFFF)); /* pass 32bit address */
        return -1;
    }
    else if (p[15] == 2)
    {
        k__printf("\nVersion 2\n");
        if (check_csum(p, *((os_uint32*)(p + 20))) != 0)
        { /* check full length*/
            k__printf("Error in RSDP extended checksum\n");
            return -1;
        }
        if (*((os_uint32 *)(p + 24)) || *((os_uint32 *)(p + 28)))
        {
            acpi_parse_xsdt((void *)*((os_uint64 *)(p + 24))); /* 64bit address */
        }
        else
        {
            acpi_parse_rsdt((void *)(os_intn)(*((os_uint32 *)(p + 16)) & 0xFFFFFFFF)); /*32bit address*/
        }
    }
    else
    {
        k__printf("Unsupported ACPI version ");
        k__printf("%x", p[15]);
    }
    return 0;
}

void acpi__init(void *tbl)
{
    os_uint8 *p, *q, *end;

    k__printf("ACPI -- \n");
    p = (os_uint8 *)tbl;
    end = p + 0x20000;
    while (p < end)
    {
        if (!sigcmp(p, "RSD "))
        {
            q = p + 4;
            if (!sigcmp(q, "PTR "))
            { /* 'RSD PTR' */
                if (!acpi_parse_rsdp(p))
                {
                    k__printf("MEs done\n");
                    break;
                }
            }
        }
        p += 16;
    }
    k__printf("endof ACPI\n");
}
