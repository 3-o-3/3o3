/******************************************************************************
 *                        OS-3o3 operating system
 *
 *             Memory Management Unit for the Raspberry PI400
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 *
 *****************************************************************************/

#include "../../include/klib.h"

int pl011__read();
void os__enable_mmu(os_uint32 table);

#define MEM_TOTAL_END ARM_GIC_END
#define PAGE_TABLE1_SIZE 0x4000
#define PAGE_SIZE 4096
#define MEM_PAGE_TABLE1 (MEM_COHERENT_REGION - PAGE_TABLE1_SIZE)
#define ARMV7LPAE_LEVEL2_BLOCK_SIZE (2 * MEGABYTE)

#define ATTRIB_AP_RW_PL1 0
#define ATTRIB_AP_RW_ALL 1
#define ATTRIB_AP_RO_PL1 2
#define ATTRIB_AP_RO_ALL 3

#define ATTRIB_SH_NON_SHAREABLE 0
#define ATTRIB_SH_OUTER_SHAREABLE 2
#define ATTRIB_SH_INNER_SHAREABLE 3

static void set_l2_value(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x3ULL << 0);
    entry[0] |= (v & 0x3ULL) << 0; /* 0*/
}

static void set_l2_AttrIndx(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x7ULL << 2);
    entry[0] |= (v & 0x7ULL) << 2; 
}

/*static void set_l2_NS(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x1ULL << 5);
    entry[0] |= (v & 0x1ULL) << 5;
}*/

static void set_l2_AP(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x3ULL << 6);
    entry[0] |= (v & 0x3ULL) << 6;
}

static void set_l2_SH(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x3ULL << 8);
    entry[0] |= (v & 0x3ULL) << 8;
}

static void set_l2_AF(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x1ULL << 10);
    entry[0] |= (v & 0x1ULL) << 10;
}

/*static void set_l2_nG(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x1ULL << 11);
    entry[0] |= (v & 0x1ULL) << 11;
}*/
/*unknow 9 bit*/

static void set_l2_output_address(os_uint64 *entry, os_uint64 addr)
{
    entry[0] &= ~(0x7FFFFULL << 21);
    entry[0] |= addr & (0x7FFFFULL << 21);
}

/*unkonwn 12bit*/

/*static void set_l2_Continous(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x1ULL << 52);
    entry[0] |= (v & 0x1ULL) << 52;
}*/

/*static void set_l2_PXN(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x1ULL << 53);
    entry[0] |= (v & 0x1ULL) << 53;
}*/
/*
static void set_l2_XN(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x1ULL << 54);
    entry[0] |= (v & 0x1ULL) << 54;
}
*/
/*ignored 9 bit*/

/*DDI0487K_a_a-profile_architecture_reference_manual.pdf
 VMSAv8-32 G5.3 Translation tables
 we are in stage 1, EL1*/
static os_uint64 *create_l2(os_uint32 n, os_uint64 a)
{
    os_uint64 *l2 = (os_uint64 *)(MEM_PAGE_TABLE1 - (PAGE_SIZE * (n + 2)));
    os_uint32 i;
    for (i = 0; i < 512; i++)
    {
        (l2 + i)[0] = a;
       
        set_l2_value(l2 + i, 1);
        set_l2_AttrIndx(l2 + i, ATTRINDX_DEVICE);
      
        set_l2_AP(l2 + i, ATTRIB_AP_RW_ALL);
        set_l2_SH(l2 + i, ATTRIB_SH_OUTER_SHAREABLE);
        set_l2_AF(l2 + i, 1);
     

        if (a == MEM_PCIE_RANGE_START_VIRTUAL)
        {
            k__printf("L2: %x:%x\n", (os_uint32)(l2[i] >> 32), (os_uint32)l2[i]);
            set_l2_output_address(l2 + i, MEM_PCIE_RANGE_START);
        }
        a += 2 * 1024 * 1024;
#if 0
		extern os_uint8 _etext;
		if (nBaseAddress >= (os_uint64)&_etext)
		{
			pDesc->PXN = 1;

			if ((nBaseAddress >= m_nMemSize
				&& nBaseAddress < MEM_HIGHMEM_START)
				|| nBaseAddress > MEM_HIGHMEM_END)
			{
				pDesc->AttrIndx = ATTRINDX_DEVICE;
				pDesc->SH = ATTRIB_SH_OUTER_SHAREABLE;
			}
			else if (nBaseAddress >= MEM_COHERENT_REGION
				&& nBaseAddress < MEM_HEAP_START)
			{
				pDesc->AttrIndx = ATTRINDX_COHERENT;
				pDesc->SH = ATTRIB_SH_OUTER_SHAREABLE;
			}
#endif
    }
    return l2;
}

/*static void set_l1_Value(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x3ULL << 0);
    entry[0] |= v & (0x3ULL << 0);
}*/

/*static void set_l1_TableAddress(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0xFFFFFFFULL << 12);
    entry[0] |= v & (0xFFFFFFFULL << 12);
}*/
/*
static void set_l1_AF(os_uint64 *entry, os_uint64 v)
{
    entry[0] &= ~(0x1ULL << 10);
    entry[0] |= v & (0x1ULL << 10);
}
*/

#define UXN (1ULL << 54)
#define PXN (1ULL << 53)
#define ACCESS_FLAG (1 << 10)
#define OUTER_SHAREABLE (2 << 8)
#define INNER_SHAREABLE (3 << 8)
#define READ_ONLY (1 << 9)
#define MEMORY_ATTR 0
#define BLOCK 1
#define TABLE 3

/*https://github.com/sean-lawless/computersystems/blob/master/Lab4%20LED/openocd_rpi_jtag.cfg*/

void mmu__init()
{
    os_uint64 *l2;
    os_uint32 i;
    os_uint64 *l1 = (os_uint64 *)(MEM_PAGE_TABLE1 /* - PAGE_SIZE*/);

    /*K__memset(l0, 0, PAGE_SIZE);
     l0[0] = ACCESS_FLAG | TABLE | (os_uint64)l1;
    */
    k__memset(l1, 0, PAGE_SIZE * 2);
    k__printf("L__: %x:%x\n", (os_uint32)((os_uint64)l1 >> 32), (os_uint32)l1);
    l1[0] = ACCESS_FLAG | INNER_SHAREABLE | MEMORY_ATTR | BLOCK;
    for (i = 0; i < 4; i++)
    {
        os_uint64 a = (os_uint64)i * 512ULL * (1024 * 1024 * 2);
        l2 = create_l2(i, a);
        /*l2 = 0;*/
        l1[i] = (os_uint64)l2 | ACCESS_FLAG | INNER_SHAREABLE | TABLE;
       
        k__printf("L1: %x:%x\n", (os_uint32)(l1[i] >> 32), (os_uint32)l1[i]);
    }

    k__printf("MIX: %x:%x\n", (os_uint32)(l1[0] >> 32), (os_uint32)l1[0]);
    k__printf("MIX: %x:%x\n", (os_uint32)(l1[1] >> 32), (os_uint32)l1[1]);
    k__printf("MIX: %x:%x\n", (os_uint32)(l1[2] >> 32), (os_uint32)l1[2]);
    k__printf("MIX: %x:%x\n", (os_uint32)(l1[3] >> 32), (os_uint32)l1[3]);
    os__data_sync_barrier();
    os__enable_mmu((os_uint32)l1);
}