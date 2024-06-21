/******************************************************************************
 *                       OS-3o3 operating system
 * 
 *                  convert ELF object to raw binary
 * 
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE (1024*1024*4) 
/*Assumes running on PC little endian*/
#ifdef ntohl
#undef ntohl
#undef ntohs
#endif
#ifdef USE_BIG_ENDIAN
#define ntohl(A) (((A)>>24)|(((A)&0x00ff0000)>>8)|(((A)&0xff00)<<8)|((A)<<24))
#define ntohs(A) (os_uint16)((((A)&0xff00)>>8)|((A)<<8))
#else
#define ntohl(A) A
#define ntohs(A) A
#endif

#define EI_NIDENT 16
#define SHT_PROGBITS 1
#define SHT_STRTAB 3
#define SHT_NOBITS 8

typedef unsigned int   os_uint32;
typedef unsigned short os_uint16;
typedef unsigned char  os_utf8;
typedef long  os_intn;

#pragma pack(1)
typedef struct
{
    os_utf8 e_ident[EI_NIDENT];
    os_uint16 e_type;
    os_uint16 e_machine;
    os_uint32 e_version;
    os_uint32 e_entry;
    os_uint32 e_phoff;
    os_uint32 e_shoff;
    os_uint32 e_flags;
    os_uint16 e_ehsize;
    os_uint16 e_phentsize;
    os_uint16 e_phnum;
    os_uint16 e_shentsize;
    os_uint16 e_shnum;
    os_uint16 e_shstrndx;
} elf__header;

typedef struct
{
    os_uint32 p_type;
    os_uint32 p_offset;
    os_uint32 p_vaddr;
    os_uint32 p_paddr;
    os_uint32 p_filesz;
    os_uint32 p_memsz;
    os_uint32 p_flags;
    os_uint32 p_align;
} elf__32_phdr;

typedef struct
{
    os_uint32 sh_name;
    os_uint32 sh_type;
    os_uint32 sh_flags;
    os_uint32 sh_addr;
    os_uint32 sh_offset;
    os_uint32 sh_size;
    os_uint32 sh_link;
    os_uint32 sh_info;
    os_uint32 sh_addralign;
    os_uint32 sh_entsize;
} elf__32_shdr;

typedef struct
{
    os_uint32 ri_gprmask;
    os_uint32 ri_cprmask[4];
    os_uint32 ri_gp_value;
} elf__reg_info;

typedef struct {
    os_uint32	st_name;
    os_uint32	st_value;
    os_uint32	st_size;
    os_utf8 st_info;
    os_utf8 st_other;
    os_uint16	st_shndx;
} elf__32_sym;

typedef struct {
    os_uint32 r_offset;
    os_uint32 r_info;
} elf__32_rel;


int main(int argc, char* argv[])
{
    os_uint32 offset = 0x07C00;
    FILE* infile, * outfile, * txtfile;
    os_utf8* buf, * code, * strtab, * symtab, * progbits, *reltab;

    os_uint32 progsize, symsize, relsize;
    os_intn stack_pointer = 0;
    os_uint32 length, i, gp_ptr = 0, gp_ptr_backup = 0;
    os_uint32 bss_start = 0;

    elf__header* elfHeader;
    elf__32_phdr* elfProgram;
    /*elf__reg_info* elfRegInfo;*/
    elf__32_shdr* elfSection;
    /*elf__32_shdr* progsection;*/
    elf__32_sym* elfSym;
    elf__32_rel* elfRel;
    (void)argc;
    (void)stack_pointer;

    printf("%s -> code.txt & test.bin\n", argv[1]);

    infile = fopen(argv[1], "rb");
    if (infile == NULL)
    {
        printf("Can't open %s\n", argv[1]);
        return 0;
    }
    buf = (os_utf8*)malloc(BUF_SIZE);
    if (!buf) return -1;
    fread(buf, 1, BUF_SIZE, infile);
    fclose(infile);
    code = (os_utf8*)malloc(BUF_SIZE);
    if (!code) return -1;
    memset(code, 0, BUF_SIZE);

    elfHeader = (elf__header*)buf;
    if (elfHeader->e_ident[0] != 0x7F || strncmp((char*)elfHeader->e_ident + 1, "ELF", 3))
    {
        printf("Error:  Not an ELF file!\n");
        return -1;
    }

    if (elfHeader->e_ident[0x4] != 1) {
        printf("Error:  Not 32 bit!\n");
        return -1;
    }

    if (elfHeader->e_ident[0x5] != 1) {
        printf("Error:  Not little endian!\n");
        return -1;
    }


    if (elfHeader->e_ident[0x6] != 1) {
        printf("Error: not ELF version 1!\n");
        return -1;
    }


    printf("EI_OSABI %x\n", elfHeader->e_ident[0x7]);
    printf("EI_ABIVERSION %x\n", elfHeader->e_ident[0x8]);

    printf("e_type %x\n", elfHeader->e_type);
    printf("e_machine %x (3 == x86)\n", elfHeader->e_machine);


    elfHeader->e_entry = ntohl(elfHeader->e_entry);
    elfHeader->e_phoff = ntohl(elfHeader->e_phoff);
    elfHeader->e_shoff = ntohl(elfHeader->e_shoff);
    elfHeader->e_flags = ntohl(elfHeader->e_flags);
    elfHeader->e_phentsize = ntohs(elfHeader->e_phentsize);
    elfHeader->e_phnum = ntohs(elfHeader->e_phnum);
    elfHeader->e_shentsize = ntohs(elfHeader->e_shentsize);
    elfHeader->e_shnum = ntohs(elfHeader->e_shnum);
    printf("Entry=0x%x %d\n", elfHeader->e_entry, elfHeader->e_shnum);
    length = 0;

    /* program header */
    for (i = 0; i < elfHeader->e_phnum; ++i)
    {
        elfProgram = (elf__32_phdr*)(buf + elfHeader->e_phoff +
            elfHeader->e_phentsize * i);
        elfProgram->p_type = ntohl(elfProgram->p_type);
        elfProgram->p_offset = ntohl(elfProgram->p_offset);
        elfProgram->p_vaddr = ntohl(elfProgram->p_vaddr);
        elfProgram->p_filesz = ntohl(elfProgram->p_filesz);
        elfProgram->p_memsz = ntohl(elfProgram->p_memsz);
        elfProgram->p_flags = ntohl(elfProgram->p_flags);

        elfProgram->p_vaddr -= elfHeader->e_entry;

       
        if ((int)elfProgram->p_vaddr < BUF_SIZE)
        {
            printf("[vaddr=0x%x,offset=0x%x,filesz=0x%x,memsz=0x%x,flags=0x%x]\n",
                elfProgram->p_vaddr, elfProgram->p_offset,
                elfProgram->p_filesz, elfProgram->p_memsz,
                elfProgram->p_flags);
            if ((int)elfProgram->p_vaddr < 0)
                elfProgram->p_vaddr = 0;
            memcpy(code + elfProgram->p_vaddr, buf + elfProgram->p_offset,
                elfProgram->p_filesz);
            length = elfProgram->p_vaddr + elfProgram->p_filesz;
            printf("length = %d = 0x%x\n", length, length);
        }
    }

    /* section header */
    strtab = (os_utf8*)"";
    symtab = (os_utf8*)"";
    progbits = NULL;
    progsize = 0;
    symsize = 0;
    reltab = NULL;
    relsize = 0;
    for (i = 0; i < elfHeader->e_shnum; ++i)
    {
        elfSection = (elf__32_shdr*)(buf + elfHeader->e_shoff +
            elfHeader->e_shentsize * i);
        elfSection->sh_name = ntohl(elfSection->sh_name);
        elfSection->sh_type = ntohl(elfSection->sh_type);
        elfSection->sh_addr = ntohl(elfSection->sh_addr);
        elfSection->sh_offset = ntohl(elfSection->sh_offset);
        elfSection->sh_size = ntohl(elfSection->sh_size);

        printf("%x elfSection->sh_addr=0x%x size=%x\n", elfSection->sh_type, elfSection->sh_addr, elfSection->sh_size);
        switch (elfSection->sh_type) {
        case 0:
            printf("SHT_NULL");
            break;
        case 1:
            printf("SHT_PROGBITS");
            progbits = (os_utf8*)(elfSection->sh_offset + buf);
            progsize = elfSection->sh_size;
            break;
        case 2:
            printf("SHT_SYMTAB");
            symtab = (os_utf8*)(elfSection->sh_offset + buf);
            symsize = elfSection->sh_size;
            break;
        case 3:
            printf("SHT_STRTAB");
            strtab = (os_utf8*)(elfSection->sh_offset + buf);
            break;
        case 9:
            printf("SHT_REL");
            reltab = (os_utf8*)(elfSection->sh_offset + buf);
            relsize = elfSection->sh_size;
            break;
        default:
            break;
        }
        printf(" %s \n", strtab + elfSection->sh_name);

        if (elfSection->sh_type == SHT_PROGBITS)
        {

            if (elfSection->sh_addr > gp_ptr_backup)
                gp_ptr_backup = elfSection->sh_addr;
        }
        if (elfSection->sh_type == SHT_NOBITS)
        {
            if (bss_start == 0)
            {
                bss_start = elfSection->sh_addr;
            }
            /*bss_end = elfSection->sh_addr + elfSection->sh_size;*/
        }
    }

    elfSym = (void*)symtab;
    while ((os_utf8*)elfSym < symtab + symsize) {
        if (elfSym->st_name) {
            printf("%s value=%x size=%x info=%x\n", strtab + elfSym->st_name, elfSym->st_value, elfSym->st_size, elfSym->st_info);
        }
        elfSym++;
    }
    elfSym = (void*)symtab;
    elfRel = (void*)reltab;
    while (((os_utf8*)elfRel < reltab + relsize) && progbits) {
        
        if ((elfRel->r_info >> 8) == 1) {
            if ((elfRel->r_info & 0xFF) == 0x14) {
                /* R_386_16 word16 S + A */
                os_uint16* ref = (os_uint16*)(progbits + elfRel->r_offset);
                elf__32_sym* s = elfSym + (elfRel->r_info >> 8);
                if (s->st_shndx == 2) {
                   *ref = (os_uint16)(*ref + s->st_value + offset);
                    printf("REL: s->%x\n", s->st_shndx);
                }
            }
            else {
                printf("REL: inf %x\n", elfRel->r_info & 0xFF);
            }
        }
        else {
            printf("UNKNOWN REL: sym=%x type=%x offset=%x\n", elfRel->r_info >> 8, elfRel->r_info & 0xFF, elfRel->r_offset);
        }
        elfRel++;
    }
    if (bss_start == 0)
        bss_start = length;
    /*if(length > bss_start - elfHeader->e_entry)
    {
       length = bss_start - elfHeader->e_entry;
    }*/
    if (bss_start == length)
    {
        bss_start = length;
        /*bss_end = length + 4;*/
    }
    if (gp_ptr == 0)
        gp_ptr = gp_ptr_backup + 0x7ff0;


    code = progbits;
    length = progsize;
    if (!code) return -1;

    /*write out test.bin */
    outfile = fopen(argv[2], "wb");
    fwrite(code, length, 1, outfile);
    fclose(outfile);

    /*write out code.txt */
    txtfile = fopen("440.bin", "wb");
    fwrite(code, 440, 1, txtfile);
    fclose(txtfile);
    free(buf);
    printf("length=%d=0x%x\n", length, length);

    return 0;
}
