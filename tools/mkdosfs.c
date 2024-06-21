/*
12.06.2024 downloaded form https://github.com/robertapengelly/dosfstools
*/
#ifdef LICENSE
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
#endif /* LICENSE */

#ifndef common_h
/******************************************************************************
 * @file            common.h
 *****************************************************************************/
#ifndef     _COMMON_H
#define     _COMMON_H

extern unsigned short generate_datestamp (void);
extern unsigned short generate_timestamp (void);

#endif      /* _COMMON_H */
#endif /* common.h  --------------------- */

#ifndef lib_h

/******************************************************************************
 * @file            lib.h
 *****************************************************************************/
#ifndef     _LIB_H
#define     _LIB_H

#include    <stddef.h>

char *xstrdup (const char *str);
int xstrcasecmp (const char *s1, const char *s2);

void *xmalloc (size_t size);
void *xrealloc (void *ptr, size_t size);

void dynarray_add (void *ptab, size_t *nb_ptr, void *data);
void parse_args (int *pargc, char ***pargv, int optind);

#endif      /* _LIB_H */
#endif /*  lib.h --------------------- */

#ifndef write7x_h
/******************************************************************************
 * @file            write7x.h
 *****************************************************************************/
#ifndef     _WRITE7X_H
#define     _WRITE7X_H

void write721_to_byte_array (unsigned char *dest, unsigned short val);
void write741_to_byte_array (unsigned char *dest, unsigned int val);

#endif      /* _WRITE7X_H */
#endif /* write7x.h --------------------- */


#ifndef report_h
/******************************************************************************
 * @file            report.h
 *****************************************************************************/
#ifndef     _REPORT_H
#define     _REPORT_H

enum {

    REPORT_ERROR = 0,
    REPORT_FATAL_ERROR,
    REPORT_INTERNAL_ERROR,
    REPORT_WARNING

};

#if     defined (_WIN32)
# define    COLOR_ERROR                 12
# define    COLOR_WARNING               13
# define    COLOR_INTERNAL_ERROR        19
#else
# define    COLOR_ERROR                 91
# define    COLOR_INTERNAL_ERROR        94
# define    COLOR_WARNING               95
#endif

void report_at (const char *filename, unsigned long line_number, int type, const char *fmt, ...);

#endif      /* _REPORT_H */
#endif /* report.h --------------------- */

#ifndef msdos_h

/******************************************************************************
 * @file            msdos.h
 *****************************************************************************/
#ifndef     _MSDOS_H
#define     _MSDOS_H

/**
 * According to Microsoft FAT specification (fatgen103.doc) disk with
 * 4085 clusters (or more) is FAT16, but Microsoft Windows FAT driver
 * fastfat.sys detects disk with less then 4087 clusters as FAT12.
 * Linux FAT drivers msdos.ko and vfat.ko detect disk with at least
 * 4085 clusters as FAT16, therefore for compatibility reasons with
 * both systems disallow formatting disks to 4085 or 4086 clusters.
 */
#define     MAX_CLUST_12                4084
#define     MIN_CLUST_16                4087

/**
 * According to Microsoft FAT specification (fatgen103.doc) disk with
 * 65525 clusters (or more) is FAT32, but Microsoft Windows FAT driver
 * fastfat.sys, Linux FAT drivers msdos.ko and vfat.ko detect disk as
 * FAT32 when Sectors Per FAT (fat_length) is set to zero. And not by
 * number of clusters. Still there is cluster upper limit for FAT16.
 */
#define     MAX_CLUST_16                65524
#define     MIN_CLUST_32                65525

/**
 * M$ says the high 4 bits of a FAT32 FAT entry are reserved and don't belong
 * to the cluster number. So the max. cluster# is based on 2^28
 */
#define     MAX_CLUST_32                268435446

/** File Attributes. */
#define     ATTR_NONE                   0x00
#define     ATTR_READONLY               0x01
#define     ATTR_HIDDEN                 0x02
#define     ATTR_SYSTEM                 0x04
#define     ATTR_VOLUME_ID              0x08
#define     ATTR_DIR                    0x10
#define     ATTR_ARCHIVE                0x20
/*#define     ATTR_LONG_NAME              (ATTR_READONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)*/

struct msdos_volume_info {

    unsigned char drive_no;
    unsigned char boot_flags;
    unsigned char ext_boot_sign;
    unsigned char volume_id[4];
    unsigned char volume_label[11];
    unsigned char fs_type[8];

};

struct msdos_boot_sector {

    unsigned char boot_jump[3];
    unsigned char system_id[8];
    unsigned char bytes_per_sector[2];
    unsigned char sectors_per_cluster;
    unsigned char reserved_sectors[2];
    unsigned char no_fats;
    unsigned char root_entries[2];
    unsigned char total_sectors16[2];
    unsigned char media_descriptor;
    unsigned char sectors_per_fat16[2];
    unsigned char sectors_per_track[2];
    unsigned char heads_per_cylinder[2];
    unsigned char hidden_sectors[4];
    unsigned char total_sectors32[4];
    
    union {
    
        struct {
        
            struct msdos_volume_info vi;
            unsigned char boot_code[448];
        
        } _oldfat;
        
        struct {
        
            unsigned char sectors_per_fat32[4];
            unsigned char flags[2];
            unsigned char version[2];
            unsigned char root_cluster[4];
            unsigned char info_sector[2];
            unsigned char backup_boot[2];
            unsigned short reserved2[6];
            
            struct msdos_volume_info vi;
            unsigned char boot_code[420];
        
        } _fat32;
    
    } fstype;
    
    unsigned char boot_sign[2];

};

#define     oldfat                      fstype._oldfat
#define     fat32                       fstype._fat32

struct fat32_fsinfo {

    unsigned char reserved1[4];
    unsigned char signature[4];
    unsigned char free_clusters[4];
    unsigned char next_cluster[4];

};

struct msdos_dirent {

    unsigned char name[11];
    unsigned char attr;
    unsigned char lcase;
    unsigned char ctime_cs;
    unsigned char ctime[2];
    unsigned char cdate[2];
    unsigned char adate[2];
    unsigned char starthi[2];
    unsigned char time[2];
    unsigned char date[2];
    unsigned char startlo[2];
    unsigned char size[4];

};

#endif      /* _MSDOS_H */

#endif /*  msdos.h --------------------- */
#ifndef mkfs_h
/******************************************************************************
 * @file            parted.h
 *****************************************************************************/
#ifndef     _PARTED_H
#define     _PARTED_H

#include    <stddef.h>

struct mkfs_state {

    const char *boot, *outfile;
    char label[12];
    
    int create, size_fat, size_fat_by_user, verbose;
    size_t blocks, offset;
    
    unsigned char sectors_per_cluster;

};

extern struct mkfs_state *state;
extern const char *program_name;

#endif      /* _PARTED_H */
#endif /*  mkfs.h --------------------- */

#ifndef common_c

/******************************************************************************
 * @file            common.c
 *****************************************************************************/
#include    <errno.h>
#include    <stdarg.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <time.h>

#ifndef     __PDOS__
# if     defined (__GNUC__)
#  include  <sys/time.h>
#  include  <unistd.h>
# else
#  include  <io.h>
# endif
#endif
#if 0
#include    "common.h"
#endif
unsigned short generate_datestamp (void) {

#if     defined (__GNUC__) && !defined (__PDOS__)
    struct timeval create_timeval;
#else
    time_t create_time;
#endif

    struct tm *ctime = NULL;
    
#if     defined (__GNUC__) && !defined (__PDOS__)
    
    if (gettimeofday (&create_timeval, 0) == 0 && create_timeval.tv_sec != (time_t) -1) {
        ctime = localtime ((time_t *) &create_timeval.tv_sec);
    }

#else

    if (time (&create_time) != 0 && create_time != 0) {
        ctime = localtime (&create_time);
    }

#endif
    
    if (ctime != NULL && ctime->tm_year >= 80 && ctime->tm_year <= 207) {
        return (unsigned short) (ctime->tm_mday + ((ctime->tm_mon + 1) << 5) + ((ctime->tm_year - 80) << 9));
    }
    
    return 1 + (1 << 5);

}

unsigned short generate_timestamp (void) {

#if     defined (__GNUC__) && !defined (__PDOS__)
    struct timeval create_timeval;
#else
    time_t create_time;
#endif

    struct tm *ctime = NULL;
    
#if     defined (__GNUC__) && !defined (__PDOS__)
    
    if (gettimeofday (&create_timeval, 0) == 0 && create_timeval.tv_sec != (time_t) -1) {
        ctime = localtime ((time_t *) &create_timeval.tv_sec);
    }

#else

    if (time (&create_time) != 0 && create_time != 0) {
        ctime = localtime (&create_time);
    }

#endif
    
    if (ctime != NULL && ctime->tm_year >= 80 && ctime->tm_year <= 207) {
        return (unsigned short) ((ctime->tm_sec >> 1) + (ctime->tm_min << 5) + (ctime->tm_hour << 11));
    }
    
    return 0;

}

#endif /*  common.c --------------------- */
#ifndef lib_c
/******************************************************************************
 * @file            lib.c
 *****************************************************************************/
#include    <ctype.h>
#include    <errno.h>
#include    <limits.h>
#include    <stddef.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#if 0
#include    "common.h"
#include    "lib.h"
#include    "mkfs.h"
#include    "report.h"
#endif
struct option {

    const char *name;
    int index, flags;

};

#define     OPTION_NO_ARG               0x0001
#define     OPTION_HAS_ARG              0x0002

enum options {

    OPTION_IGNORED = 0,
    OPTION_BLOCKS,
    OPTION_BOOT,
    OPTION_FAT,
    OPTION_HELP,
    OPTION_NAME,
    OPTION_OFFSET,
    OPTION_SECTORS,
    OPTION_VERBOSE

};

static struct option opts[] = {

    { "F",          OPTION_FAT,         OPTION_HAS_ARG  },
    { "n",          OPTION_NAME,        OPTION_HAS_ARG  },
    { "s",          OPTION_SECTORS,     OPTION_HAS_ARG  },
    { "v",          OPTION_VERBOSE,     OPTION_NO_ARG   },
    
    { "-boot",      OPTION_BOOT,        OPTION_HAS_ARG  },
    { "-blocks",    OPTION_BLOCKS,      OPTION_HAS_ARG  },
    { "-help",      OPTION_HELP,        OPTION_NO_ARG   },
    { "-offset",    OPTION_OFFSET,      OPTION_HAS_ARG  },
    
    { 0,            0,                  0               }

};

static int strstart (const char *val, const char **str) {

    const char *p = val;
    const char *q = *str;
    
    while (*p != '\0') {
    
        if (*p != *q) {
            return 0;
        }
        
        ++p;
        ++q;
    
    }
    
    *str = q;
    return 1;

}

static void print_help (void) {

    if (!program_name) {
        goto _exit;
    }
    
    fprintf (stderr, "Usage: %s [options] file\n\n", program_name);
    fprintf (stderr, "Options:\n\n");
    
    fprintf (stderr, "    Short options:\n\n");
    fprintf (stderr, "        -F BITS           Select FAT size BITS (12, 16, or 32).\n");
    fprintf (stderr, "        -n LABEL          Set volume label as LABEL (max 11 characters).\n");
    fprintf (stderr, "        -v                Verbose execution.\n");
    
    fprintf (stderr, "\n");
    
    fprintf (stderr, "    Long options:\n\n");
    fprintf (stderr, "        --blocks BLOCKS   Make the filesystem the size of BLOCKS * 1024.\n");
    fprintf (stderr, "        --boot FILE       Use FILE as the boot sector.\n");
    fprintf (stderr, "        --help            Show this help information then exit.\n");
    fprintf (stderr, "        --offset SECTOR   Write the filesystem starting at SECTOR.\n");
    
_exit:
    
    exit (EXIT_SUCCESS);

}

static int is_label_char (int ch) {
    return ((ch & 0x80) || (ch == 0x20) || isalnum (ch) || strchr ("!#$%'-@_{}~", ch));
}

char *xstrdup (const char *str) {

    char *ptr = xmalloc (strlen (str) + 1);
    strcpy (ptr, str);
    
    return ptr;

}

int xstrcasecmp (const char *s1, const char *s2) {

    const unsigned char *p1;
    const unsigned char *p2;
    
    p1 = (const unsigned char *) s1;
    p2 = (const unsigned char *) s2;
    
    while (*p1 != '\0') {
    
        if (toupper (*p1) < toupper (*p2)) {
            return (-1);
        } else if (toupper (*p1) > toupper (*p2)) {
            return (1);
        }
        
        p1++;
        p2++;
    
    }
    
    if (*p2 == '\0') {
        return (0);
    } else {
        return (-1);
    }

}

void *xmalloc (size_t size) {

    void *ptr = malloc (size);
    
    if (ptr == NULL && size) {
    
        report_at (program_name, 0, REPORT_ERROR, "memory full (malloc)");
        exit (EXIT_FAILURE);
    
    }
    
    memset (ptr, 0, size);
    return ptr;

}

void *xrealloc (void *ptr, size_t size) {

    void *new_ptr = realloc (ptr, size);
    
    if (new_ptr == NULL && size) {
    
        report_at (program_name, 0, REPORT_ERROR, "memory full (realloc)");
        exit (EXIT_FAILURE);
    
    }
    
    return new_ptr;

}

void dynarray_add (void *ptab, size_t *nb_ptr, void *data) {

    int nb, nb_alloc;
    void **pp;
    
    nb = *nb_ptr;
    pp = *(void ***) ptab;
    
    if ((nb & (nb - 1)) == 0) {
    
        if (!nb) {
            nb_alloc = 1;
        } else {
            nb_alloc = nb * 2;
        }
        
        pp = xrealloc (pp, nb_alloc * sizeof (void *));
        *(void ***) ptab = pp;
    
    }
    
    pp[nb++] = data;
    *nb_ptr = nb;

}

void parse_args (int *pargc, char ***pargv, int optind) {

    char **argv = *pargv;
    int argc = *pargc;
    
    struct option *popt;
    const char *optarg, *r;
    
    if (argc == optind) {
        print_help ();
    }
    
    memset (state->label, ' ', 11);
    
    while (optind < argc) {
    
        r = argv[optind++];
        
        if (r[0] != '-' || r[1] == '\0') {
        
            if (state->outfile) {
            
                report_at (program_name, 0, REPORT_ERROR, "multiple output files provided");
                exit (EXIT_FAILURE);
            
            }
            
            state->outfile = xstrdup (r);
            continue;
        
        }
        
        for (popt = opts; popt; ++popt) {
        
            const char *p1 = popt->name;
            const char *r1 = (r + 1);
            
            if (!p1) {
            
                report_at (program_name, 0, REPORT_ERROR, "invalid option -- '%s'", r);
                exit (EXIT_FAILURE);
            
            }
            
            if (!strstart (p1, &r1)) {
                continue;
            }
            
            optarg = r1;
            
            if (popt->flags & OPTION_HAS_ARG) {
            
                if (*optarg == '\0') {
                
                    if (optind >= argc) {
                    
                        report_at (program_name, 0, REPORT_ERROR, "argument to '%s' is missing", r);
                        exit (EXIT_FAILURE);
                    
                    }
                    
                    optarg = argv[optind++];
                
                }
            
            } else if (*optarg != '\0') {
                continue;
            }
            
            break;
        
        }
        
        switch (popt->index) {
        
            case OPTION_BLOCKS: {
            
                long conversion;
                char *temp;
                
                errno = 0;
                conversion = strtol (optarg, &temp, 0);
                
                if (!*optarg || isspace ((int) *optarg) || errno || *temp) {
                
                    report_at (program_name, 0, REPORT_ERROR, "bad number for blocks");
                    exit (EXIT_FAILURE);
                
                }
                
                if (conversion < 0) {
                
                    report_at (program_name, 0, REPORT_ERROR, "blocks must be greater than zero");
                    exit (EXIT_FAILURE);
                
                }
                
                state->blocks = conversion;
                break;
            
            }
            
            case OPTION_BOOT: {
            
                state->boot = xstrdup (optarg);
                break;
            
            }
            
            case OPTION_FAT: {
            
                long conversion;
                char *temp;
                
                errno = 0;
                conversion = strtol (optarg, &temp, 0);
                
                if (!*optarg || isspace ((int) *optarg) || errno || *temp) {
                
                    report_at (program_name, 0, REPORT_ERROR, "bad number for fat size");
                    exit (EXIT_FAILURE);
                
                }
                
                switch (conversion) {
                
                    case 12:
                    case 16:
                    case 32:
                    
                        break;
                    
                    default:
                    
                        report_at (program_name, 0, REPORT_ERROR, "fat size can either be 12, 16 or 32 bits");
                        exit (EXIT_FAILURE);
                
                }
                
                state->size_fat = conversion;
                state->size_fat_by_user = 1;
                
                break;
            
            }
            
            case OPTION_HELP: {
            
                print_help ();
                break;
            
            }
            
            case OPTION_NAME: {
            
                int n;
                
                if (strlen (optarg) > 11) {
                
                    report_at (program_name, 0, REPORT_ERROR, "volume label cannot exceed 11 characters");
                    exit (EXIT_FAILURE);
                
                }
                
                for (n = 0; optarg[n] != '\0'; n++) {
                
                    if (!is_label_char (optarg[n])) {
                    
                        report_at (program_name, 0, REPORT_ERROR, "volume label contains an invalid character");
                        exit (EXIT_FAILURE);
                    
                    }
                    
                    state->label[n] = toupper (optarg[n]);
                
                }
                
                break;
            
            }
            
            case OPTION_OFFSET: {
            
                long conversion;
                char *temp;
                
                errno = 0;
                conversion = strtol (optarg, &temp, 0);
                
                if (!*optarg || isspace ((int) *optarg) || *temp || errno) {
                
                    report_at (program_name, 0, REPORT_ERROR, "bad number for offset");
                    exit (EXIT_FAILURE);
                
                }
                
                if (conversion < 0 || (unsigned long) conversion > UINT_MAX) {
                
                    report_at (program_name, 0, REPORT_ERROR, "offset must be between 0 and %u", UINT_MAX);
                    exit (EXIT_FAILURE);
                
                }
                
                state->offset = (size_t) conversion;
                break;
            
            }
            
            case OPTION_SECTORS: {
            
                long conversion;
                char *temp;
                
                errno = 0;
                conversion = strtol (optarg, &temp, 0);
                
                if (!*optarg || isspace ((int) *optarg) || *temp || errno) {
                
                    report_at (program_name, 0, REPORT_ERROR, "bad number for sectors per cluster");
                    exit (EXIT_FAILURE);
                
                }
                
                if (conversion != 1 && conversion != 2 && conversion != 4 && conversion != 8 && conversion != 16 && conversion != 32 && conversion != 64 && conversion != 128) {
                
                    report_at (program_name, 0, REPORT_ERROR, "bad number for sectors per cluster");
                    exit (EXIT_FAILURE);
                
                }
                
                state->sectors_per_cluster = (unsigned char) conversion;
                break;
            
            }
            
            case OPTION_VERBOSE: {
            
                state->verbose++;
                break;
            
            }
            
            default: {
            
                report_at (program_name, 0, REPORT_ERROR, "unsupported option '%s'", r);
                exit (EXIT_FAILURE);
            
            }
        
        }
    
    }
    
    if (memcmp (state->label, "           ", 11) == 0) {
        memcpy (state->label, "NO NAME    ", 11);
    }

}
#endif /*  lib.c --------------------- */
#ifndef mkfs_c

/******************************************************************************
 * @file            mkfs.c
 *****************************************************************************/
#include    <limits.h>
#include    <stddef.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#if 0
#include    "common.h"
#include    "lib.h"
#include    "mkfs.h"
#include    "msdos.h"
#include    "report.h"
#include    "write7x.h"
#endif

#ifndef     __PDOS__
# if     defined (__GNUC__)
#  include  <sys/time.h>
#  include  <unistd.h>
# else
#  include  <io.h>
# endif
#endif

static int align_structures = 1;
static int orphaned_sectors = 0;

static FILE *ofp;
static size_t image_size = 0;

static long total_sectors = 0;

static int heads_per_cylinder = 255;
static int sectors_per_track = 63;

static unsigned int backup_boot = 0;
static unsigned int cluster_count = 0;
static unsigned int hidden_sectors = 0;
static unsigned int info_sector = 0;
static unsigned int media_descriptor = 0xf8;
static unsigned int number_of_fats = 2;
static unsigned int reserved_sectors = 0;
static unsigned int root_cluster = 2;
static unsigned int root_entries = 512;
static unsigned int sectors_per_cluster = 4;
static unsigned int sectors_per_fat = 0;

struct mkfs_state *state = 0;
const char *program_name = 0;

static unsigned char dummy_boot_code[] =

    "\x31\xC0"                                                                  /* xor ax, ax */
    "\xFA"                                                                      /* cli */
    "\x8E\xD0"                                                                  /* mov ss, ax */
    "\xBC\x00\x7C"                                                              /* mov sp, 0x7c00 */
    "\xFB"                                                                      /* sti */
    "\x0E\x1F"                                                                  /* push cs, pop ds */
    "\xEB\x19"                                                                  /* jmp XSTRING */
    
    "\x5E"                                                                      /* PRN: pop si */
    "\xFC"                                                                      /* cld */
    "\xAC"                                                                      /* XLOOP: lodsb */
    "\x08\xC0"                                                                  /* or al, al */
    "\x74\x09"                                                                  /* jz EOF */
    "\xB4\x0E"                                                                  /* mov ah, 0x0e */
    "\xBB\x07\x00"                                                              /* mov bx, 7 */
    "\xCD\x10"                                                                  /* int 0x10 */
    "\xEB\xF2"                                                                  /* jmp short XLOOP */
    
    "\x31\xC0"                                                                  /* EOF: xor ax, ax */
    "\xCD\x16"                                                                  /* int 0x16 */
    "\xCD\x19"                                                                  /* int 0x19 */
    "\xF4"                                                                      /* HANG: hlt */
    "\xEB\xFD"                                                                  /* jmp short HANG */
    
    "\xE8\xE4\xFF"                                                              /* XSTRINGL call PRN */
    
    "Non-System disk or disk read error\r\n"
    "Replace and strike any key when ready\r\n";

static int cdiv (int a, int b) {
    return (a + b - 1) / b;
}

static int seekto (long offset) {
    return fseek (ofp, (state->offset * 512) + offset, SEEK_SET);
}

static int set_fat_entry (unsigned int cluster, unsigned int value) {

    unsigned char *scratch;
    unsigned int i, offset, sector;
    
    if (!(scratch = (unsigned char *) malloc (512))) {
        return -1;
    }
    
    if (state->size_fat == 12) {
    
        offset = cluster + (cluster / 2);
        value &= 0x0fff;
    
    } else if (state->size_fat == 16) {
    
        offset = cluster * 2;
        value &= 0xffff;
    
    } else if (state->size_fat == 32) {
    
        offset = cluster * 4;
        value &= 0x0fffffff;
    
    } else {
    
        free (scratch);
        return -1;
    
    }
    
    /**
     * At this point, offset is the BYTE offset of the desired sector from the start
     * of the FAT.  Calculate the physical sector containing this FAT entry.
     */
    sector = (offset / 512) + reserved_sectors;
    
    if (seekto (sector * 512)) {
    
        free (scratch);
        
        report_at (program_name, 0, REPORT_ERROR, "failed whilst seeking %s", state->outfile);
        return -1;
    
    }
    
    if (fread (scratch, 512, 1, ofp) != 1) {
    
        free (scratch);
        
        report_at (program_name, 0, REPORT_ERROR, "failed whilst reading %s", state->outfile);
        return -1;
    
    }
    
    /**
     * At this point, we "merely" need to extract the relevant entry.  This is
     * easy for FAT16 and FAT32, but a royal PITA for FAT12 as a single entry
     * may span a sector boundary.  The normal way around this is always to
     * read two FAT sectors, but luxary is (by design intent) unavailable.
     */
    offset %= 512;
    
    if (state->size_fat == 12) {
    
        if (offset == 511) {
        
            if (((cluster * 3) & 0x01) == 0) {
                scratch[offset] = (unsigned char) (value & 0xFF);
            } else {
                scratch[offset] = (unsigned char) ((scratch[offset] & 0x0F) | (value & 0xF0));
            }
            
            for (i = 0; i < number_of_fats; i++) {
            
                long temp = sector + (i * sectors_per_fat);
                
                if (seekto (temp * 512) || fwrite (scratch, 512, 1, ofp) != 1) {
                
                    free (scratch);
                    return -1;
                
                }
            
            }
            
            sector++;
            
            if (seekto (sector * 512) || fread (scratch, 512, 1, ofp) != 1) {
            
                free (scratch);
                return -1;
            
            }
            
            if (((cluster * 3) & 0x01) == 0) {
                scratch[0] = (unsigned char) ((scratch[0] & 0xF0) | (value & 0x0F));
            } else {
                scratch[0] = (unsigned char) (value & 0xFF00);
            }
            
            goto _write_fat;
        
        } else {
        
            if (((cluster * 3) & 0x01) == 0) {
            
                scratch[offset] = (unsigned char) (value & 0x00FF);
                scratch[offset + 1] = (unsigned char) ((scratch[offset + 1] & 0x00F0) | ((value & 0x0F00) >> 8));
            
            } else {
            
                scratch[offset] = (unsigned char) ((scratch[offset] & 0x000F) | ((value & 0x000F) << 4));
                scratch[offset + 1] = (unsigned char) ((value & 0x0FF0) >> 4);
            
            }
            
            goto _write_fat;
        
        }
    
    } else if (state->size_fat == 16) {
    
        scratch[offset] = (value & 0xFF);
        scratch[offset + 1] = (value >> 8) & 0xFF;
        
        goto _write_fat;
    
    } else if (state->size_fat == 32) {
    
        scratch[offset] = (value & 0xFF);
        scratch[offset + 1] = (value >> 8) & 0xFF;
        scratch[offset + 2] = (value >> 16) & 0xFF;
        scratch[offset + 3] = (scratch[offset + 3] & 0xF0) | ((value >> 24) & 0xFF);
        
        goto _write_fat;
    
    }
    
    free (scratch);
    return -1;

_write_fat:

    for (i = 0; i < number_of_fats; i++) {
    
        long temp = sector + (i * sectors_per_fat);
        
        if (seekto (temp * 512) || fwrite (scratch, 512, 1, ofp) != 1) {
        
            free (scratch);
            return -1;
        
        }
    
    }
    
    free (scratch);
    return 0;

}

static unsigned int align_object (unsigned int sectors, unsigned int clustsize) {

    if (align_structures) {
        return (sectors + clustsize - 1) & ~(clustsize - 1);
    }
    
    return sectors;

}

static unsigned int generate_volume_id (void) {

#if     defined (__PDOS__)

    srand (time (NULL));
    
    /* rand() returns int from [0,RAND_MAX], therefor only 31-bits. */
    return (((unsigned int) (rand () & 0xFFFF)) << 16) | ((unsigned int) (rand() & 0xFFFF));

#elif   defined (__GNUC__)

    struct timeval now;
    
    if (gettimeofday (&now, 0) != 0 || now.tv_sec == (time_t) -1 || now.tv_sec < 0) {
    
        srand (getpid ());
        
        /*- rand() returns int from [0,RAND_MAX], therefor only 31-bits. */
        return (((unsigned int) (rand () & 0xFFFF)) << 16) | ((unsigned int) (rand() & 0xFFFF));
    
    }
    
    /* volume id = current time, fudged for more uniqueness. */
    return ((unsigned int) now.tv_sec << 20) | (unsigned int) now.tv_usec;

#elif   defined (__WATCOMC__)

    srand (getpid ());
    
    /* rand() returns int from [0,RAND_MAX], therefor only 31-bits. */
    return (((unsigned int) (rand () & 0xFFFF)) << 16) | ((unsigned int) (rand() & 0xFFFF));

#else 

    srand (time (NULL));
    
    /* rand() returns int from [0,RAND_MAX], therefor only 31-bits. */
    return (((unsigned int) (rand () & 0xFFFF)) << 16) | ((unsigned int) (rand() & 0xFFFF));

#endif

}

static void establish_bpb (void) {

    unsigned int maxclustsize, root_dir_sectors;
    
    unsigned int clust12, clust16, clust32;
    unsigned int fatdata1216, fatdata32;
    unsigned int fatlength12, fatlength16, fatlength32;
    unsigned int maxclust12, maxclust16, maxclust32;
    
    int cylinder_times_heads;
    total_sectors = (image_size / 512) + orphaned_sectors;
    
    if ((unsigned long) total_sectors > UINT_MAX) {
    
        report_at (program_name, 0, REPORT_WARNING, "target too large, space at end will be left unused.\n");
        total_sectors = UINT_MAX;
    
    }
    
    if (total_sectors > (long) 65535 * 16 * 63) {
    
        heads_per_cylinder = 255;
        sectors_per_track = 63;
        
        cylinder_times_heads = total_sectors / sectors_per_track;
    
    } else {
    
        sectors_per_track = 17;
        cylinder_times_heads = total_sectors / sectors_per_track;
        
        heads_per_cylinder = (cylinder_times_heads + 1023) >> 10;
        
        if (heads_per_cylinder < 4) {
            heads_per_cylinder = 4;
        }
        
        if (cylinder_times_heads >= heads_per_cylinder << 10 || heads_per_cylinder > 16) {
        
            sectors_per_track = 31;
            heads_per_cylinder = 16;
            
            cylinder_times_heads = total_sectors / sectors_per_track;
        
        }
        
        if (cylinder_times_heads >= heads_per_cylinder << 10) {
        
            sectors_per_track = 63;
            heads_per_cylinder = 16;
            
            cylinder_times_heads = total_sectors / sectors_per_track;
        
        }
    
    }
    
    switch (total_sectors) {
    
        case 320:                                                               /* 160KB 5.25" */
        
            sectors_per_cluster = 2;
            root_entries = 112;
            media_descriptor = 0xfe;
            sectors_per_track = 8;
            heads_per_cylinder = 1;
            break;
        
        case 360:                                                               /* 180KB 5.25" */
        
            sectors_per_cluster = 2;
            root_entries = 112;
            media_descriptor = 0xfc;
            sectors_per_track = 9;
            heads_per_cylinder = 1;
            break;
        
        case 640:                                                               /* 320KB 5.25" */
        
            sectors_per_cluster = 2;
            root_entries = 112;
            media_descriptor = 0xff;
            sectors_per_track = 8;
            heads_per_cylinder = 2;
            break;
        
        case 720:                                                               /* 360KB 5.25" */
        
            sectors_per_cluster = 2;
            root_entries = 112;
            media_descriptor = 0xfd;
            sectors_per_track = 9;
            heads_per_cylinder = 2;
            break;
        
        case 1280:                                                              /* 640KB 5.25" / 3.5" */
        
            sectors_per_cluster = 2;
            root_entries = 112;
            media_descriptor = 0xfb;
            sectors_per_track = 8;
            heads_per_cylinder = 2;
            break;
        
        case 1440:                                                              /* 720KB 5.25" / 3.5" */
        
            sectors_per_cluster = 2;
            root_entries = 112;
            media_descriptor = 0xf9;
            sectors_per_track = 9;
            heads_per_cylinder = 2;
            break;
        
        case 1640:                                                              /* 820KB 3.5" */
        
            sectors_per_cluster = 2;
            root_entries = 112;
            media_descriptor = 0xf9;
            sectors_per_track = 10;
            heads_per_cylinder = 2;
            break;
        
        case 2400:                                                              /* 1.20MB 5.25" / 3.5" */
        
            sectors_per_cluster = 1;
            root_entries = 224;
            media_descriptor = 0xf9;
            sectors_per_track = 15;
            heads_per_cylinder = 2;
            break;
        
        case 2880:                                                              /* 1.44MB 3.5" */
        
            sectors_per_cluster = 1;
            root_entries = 224;
            media_descriptor = 0xf0;
            sectors_per_track = 18;
            heads_per_cylinder = 2;
            break;
        
        case 3360:                                                              /* 1.68MB 3.5" */
        
            sectors_per_cluster = 1;
            root_entries = 224;
            media_descriptor = 0xf0;
            sectors_per_track = 21;
            heads_per_cylinder = 2;
            break;
        
        case 3444:                                                              /* 1.72MB 3.5" */
        
            sectors_per_cluster = 1;
            root_entries = 224;
            media_descriptor = 0xf0;
            sectors_per_track = 21;
            heads_per_cylinder = 2;
            break;
        
        case 5760:                                                              /* 2.88MB 3.5" */
        
            sectors_per_cluster = 2;
            root_entries = 240;
            media_descriptor = 0xf0;
            sectors_per_track = 36;
            heads_per_cylinder = 2;
            break;
    
    }
    
    if (!state->size_fat && image_size >= (long) 512 * 1024 * 1024) {
        state->size_fat = 32;
    }
    
    if (state->size_fat == 32) {
    
        root_entries = 0;
        
        /*
         * For FAT32, try to do the same as M$'s format command
         * (see http://www.win.tue.nl/~aeb/linux/fs/fat/fatgen103.pdf p. 20):
         * 
         * fs size <= 260M: 0.5k clusters
         * fs size <=   8G:   4k clusters
         * fs size <=  16G:   8k clusters
         * fs size <=  32G:  16k clusters
         * fs size >   32G:  32k clusters
         */
        sectors_per_cluster = (total_sectors > 32 * 1024 * 1024 * 2 ? 64 :
                               total_sectors > 16 * 1024 * 1024 * 2 ? 32 :
                               total_sectors >  8 * 1024 * 1024 * 2 ? 16 :
                               total_sectors >       260 * 1024 * 2 ?  8 : 1);
    
    }
    
    if (state->sectors_per_cluster) {
        sectors_per_cluster = state->sectors_per_cluster;
    }
    
    hidden_sectors = state->offset;
    
    if (!reserved_sectors) {
        reserved_sectors = (state->size_fat == 32 ? 32 : 1);
    }
    
    /*if (align_structures) {*/
    
        /** Align number of sectors to be multiple of sectors per track, needed by DOS and mtools. */
        /*total_sectors = total_sectors / sectors_per_track * sectors_per_track;*/
    
    /*}*/
    
    if (total_sectors <= 8192) {
    
        if (align_structures && state->verbose) {
            report_at (program_name, 0, REPORT_WARNING, "Disabling alignment due to tiny filsystem\n");
        }
        
        align_structures = 0;
    
    }
    
    maxclustsize = 128;
    root_dir_sectors = cdiv (root_entries * 32, 512);
    
    do {
    
        fatdata32 = total_sectors - align_object (reserved_sectors, sectors_per_cluster);
        fatdata1216 = fatdata32 - align_object (root_dir_sectors, sectors_per_cluster);
        
        if (state->verbose) {
            fprintf (stderr, "Trying with %d sectors/cluster:\n", sectors_per_cluster);
        }
        
        /**
         * The factor 2 below avoids cut-off errors for number_of_fats == 1.
         * The "number_of_fats * 3" is for the reserved first two FAT entries.
         */
        clust12 = 2 * ((long) fatdata1216 * 512 + number_of_fats * 3) / (2 * (int) sectors_per_cluster * 512 + number_of_fats * 3);
        fatlength12 = cdiv (((clust12 + 2) * 3 + 1) >> 1, 512);
        fatlength12 = align_object (fatlength12, sectors_per_cluster);
        
        /**
         * Need to recalculate number of clusters, since the unused parts of the
         * FATs and data area together could make up space for an additional,
         * not really present cluster.
         */
        clust12 = (fatdata1216 - number_of_fats * fatlength12) / sectors_per_cluster;
        maxclust12 = (fatlength12 * 2 * 512) / 3;
        
        if (maxclust12 > MAX_CLUST_12) {
            maxclust12 = MAX_CLUST_12;
        }
        
        if (state->verbose && (state->size_fat == 0 || state->size_fat == 12)) {
            fprintf (stderr, "Trying FAT12: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n", clust12, fatlength12, maxclust12, MAX_CLUST_12);
        }
        
        if (clust12 > maxclust12) {
        
            clust12 = 0;
            
            if (state->verbose && (state->size_fat == 0 || state->size_fat == 12)) {
                fprintf (stderr, "Trying FAT12: too many clusters\n");
            }
        
        }
        
        clust16 = ((long) fatdata1216 * 512 + number_of_fats * 4) / ((int) sectors_per_cluster * 512 + number_of_fats * 2);
        fatlength16 = cdiv ((clust16 + 2) * 2, 512);
        fatlength16 = align_object (fatlength16, sectors_per_cluster);
        
        /**
         * Need to recalculate number of clusters, since the unused parts of the
         * FATs and data area together could make up space for an additional,
         * not really present cluster.
         */
        clust16 = (fatdata1216 - number_of_fats * fatlength16) / sectors_per_cluster;
        maxclust16 = (fatlength16 * 512) / 2;
        
        if (maxclust16 > MAX_CLUST_16) {
            maxclust16 = MAX_CLUST_16;
        }
        
        if (state->verbose && (state->size_fat == 0 || state->size_fat == 16)) {
            fprintf (stderr, "Trying FAT16: #clu=%u, fatlen=%u, maxclu=%u, limit=%u/%u\n", clust16, fatlength16, maxclust16, MIN_CLUST_16, MAX_CLUST_16);
        }
        
        if (clust16 > maxclust16) {
        
            clust16 = 0;
            
            if (state->verbose && (state->size_fat == 0 || state->size_fat == 16)) {
                fprintf (stderr, "Trying FAT16: too many clusters\n");
            }
        
        }
        
        /** This avoids that the filesystem will be misdetected as having a 12-bit FAT. */
        if (clust16 && clust16 < MIN_CLUST_16) {
        
            clust16 = 0;
            
            if (state->verbose && (state->size_fat == 0 || state->size_fat == 16)) {
                fprintf (stderr, "Trying FAT16: not enough clusters, would be misdected as FAT12\n");
            }
        
        }
        
        clust32 = ((long) fatdata32 * 512 + number_of_fats * 8) / ((int) sectors_per_cluster * 512 + number_of_fats * 4);
        fatlength32 = cdiv ((clust32 + 2) * 4, 512);
        fatlength32 = align_object (fatlength32, sectors_per_cluster);
        
        /**
         * Need to recalculate number of clusters, since the unused parts of the
         * FATs and data area together could make up space for an additional,
         * not really present cluster.
         */
        clust32 = (fatdata32 - number_of_fats * fatlength32) / sectors_per_cluster;
        maxclust32 = (fatlength32 * 512) / 4;
        
        if (maxclust32 > MAX_CLUST_32) {
            maxclust32 = MAX_CLUST_32;
        }
        
        if (state->verbose && (state->size_fat == 0 || state->size_fat == 32)) {
            fprintf (stderr, "Trying FAT32: #clu=%u, fatlen=%u, maxclu=%u, limit=%u/%u\n", clust32, fatlength32, maxclust32, MIN_CLUST_32, MAX_CLUST_32);
        }
        
        if (clust32 > maxclust32) {
        
            clust32 = 0;
            
            if (state->verbose && (state->size_fat == 0 || state->size_fat == 32)) {
                fprintf (stderr, "Trying FAT32: too many clusters\n");
            }
        
        }
        
        if (clust32 && clust32 < MIN_CLUST_32 && !(state->size_fat_by_user && state->size_fat == 32)) {
        
            clust32 = 0;
            
            if (state->verbose && (state->size_fat == 0 || state->size_fat == 32)) {
                fprintf (stderr, "Trying FAT32: not enough clusters\n");
            }
        
        }
        
        if ((clust12 && (state->size_fat == 0 || state->size_fat == 12)) || (clust16 && (state->size_fat == 0 || state->size_fat == 16)) || (clust32 && state->size_fat == 32)) {
            break;
        }
        
        sectors_per_cluster <<= 1;
    
    } while (sectors_per_cluster && sectors_per_cluster <= maxclustsize);
    
    /** Use the optimal FAT size if not specified. */
    if (!state->size_fat) {
    
        state->size_fat = (clust16 > clust12 ? 16 : 12);
        
        if (state->verbose) {
            report_at (program_name, 0, REPORT_WARNING, "Choosing %d-bits for FAT\n", state->size_fat);
        }
    
    }
    
    switch (state->size_fat) {
    
        case 12:
        
            cluster_count = clust12;
            sectors_per_fat = fatlength12;
            break;
        
        case 16:
        
            cluster_count = clust16;
            sectors_per_fat = fatlength16;
            break;
        
        case 32:
        
            cluster_count = clust32;
            sectors_per_fat = fatlength32;
            break;
        
        default:
        
            report_at (program_name, 0, REPORT_ERROR, "FAT not 12, 16 or 32 bits");
            
            fclose (ofp);
            remove (state->outfile);
            
            exit (EXIT_FAILURE);
    
    }
    
    /** Adjust the reserved number of sectors for alignment. */
    reserved_sectors = align_object (reserved_sectors, sectors_per_cluster);
    
    /** Adjust the number of root directory entries to help enforce alignment. */
    if (align_structures) {
        root_entries = align_object (root_dir_sectors, sectors_per_cluster) * (512 >> 5);
    }
    
    if (state->size_fat == 32) {
    
        if (!info_sector) {
            info_sector = 1;
        }
        
        if (!backup_boot) {
        
            if (reserved_sectors >= 7 && info_sector != 6) {
                backup_boot = 6;
            } else if (reserved_sectors > 3 + info_sector && info_sector != reserved_sectors - 2 && info_sector != reserved_sectors - 1) {
                backup_boot = reserved_sectors - 2;
            } else if (reserved_sectors >= 3 && info_sector != reserved_sectors - 1) {
                backup_boot = reserved_sectors - 1;
            }
        
        }
        
        if (backup_boot) {
        
            if (backup_boot == info_sector) {
            
                report_at (program_name, 0, REPORT_ERROR, "Backup boot sector must not be the same as the info sector (%d)", info_sector);
                
                fclose (ofp);
                remove (state->outfile);
                
                exit (EXIT_FAILURE);
            
            } else if (backup_boot >= reserved_sectors) {
            
                report_at (program_name, 0, REPORT_ERROR, "Backup boot sector must be a reserved sector");
                
                fclose (ofp);
                remove (state->outfile);
                
                exit (EXIT_FAILURE);
            
            }
        
        }
        
        if (state->verbose) {
            fprintf (stderr, "Using sector %d as backup boot sector (0 = none)\n", backup_boot);
        }
    
    }
    
    if (!cluster_count) {
    
        report_at (program_name, 0, REPORT_ERROR, "Not enough clusters to make a viable filesystem");
        
        fclose (ofp);
        remove (state->outfile);
        
        exit (EXIT_FAILURE);
    
    }

}

static void add_volume_label (void) {

    struct msdos_dirent *de;
    unsigned short date, time;
    
    unsigned char *scratch;
    long offset = 0;
    
    if (!(scratch = (unsigned char *) malloc (512))) {
    
        report_at (program_name, 0, REPORT_ERROR, "Failed to allocate memory");
        
        fclose (ofp);
        remove (state->outfile);
        
        exit (EXIT_FAILURE);
    
    }
    
    if (state->size_fat == 32) {
    
        long temp = reserved_sectors + (sectors_per_fat * 2);
        offset += temp + ((root_cluster - 2) * sectors_per_cluster);
    
    } else {
        offset += reserved_sectors + (sectors_per_fat * 2);
    }
    
    if (seekto (offset * 512) || fwrite (scratch, 512, 1, ofp) != 1) {
    
        report_at (program_name, 0, REPORT_ERROR, "Failed whilst reading root directory");
        free (scratch);
        
        fclose (ofp);
        remove (state->outfile);
        
        exit (EXIT_FAILURE);
    
    }
    
    de = (struct msdos_dirent *) scratch;
    memset (de, 0, sizeof (*de));
    
    date = generate_datestamp ();
    time = generate_timestamp ();
    
    memcpy (de->name, state->label, 11);
    de->attr = ATTR_VOLUME_ID;
    
    write721_to_byte_array (de->ctime, time);
    write721_to_byte_array (de->cdate, date);
    write721_to_byte_array (de->adate, date);
    write721_to_byte_array (de->time, time);
    write721_to_byte_array (de->date, date);
    
    if (seekto (offset * 512) || fwrite (scratch, 512, 1, ofp) != 1) {
    
        report_at (program_name, 0, REPORT_ERROR, "Failed whilst writing root directory");
        free (scratch);
        
        fclose (ofp);
        remove (state->outfile);
        
        exit (EXIT_FAILURE);
    
    }
    
    free (scratch);

}

static void wipe_target (void) {

    unsigned int i, sectors_to_wipe = 0;
    void *blank;
    
    if (!(blank = malloc (512))) {
    
        report_at (program_name, 0, REPORT_ERROR, "Failed to allocate memory");
        
        fclose (ofp);
        remove (state->outfile);
        
        exit (EXIT_FAILURE);
    
    }
    
    memset (blank, 0, 512);
    
    sectors_to_wipe += reserved_sectors;
    sectors_to_wipe += sectors_per_fat * 2;
    
    if (root_entries) {
        sectors_to_wipe += (root_entries * 32) / 512;
    } else {
        sectors_to_wipe += 1;
    }
    
    seekto (0);
    
    for (i = 0; i < sectors_to_wipe; i++) {
    
        if (fwrite (blank, 512, 1, ofp) != 1) {
        
            report_at (program_name, 0, REPORT_ERROR, "Failed whilst writing blank sector");
            free (blank);
            
            fclose (ofp);
            remove (state->outfile);
            
            exit (EXIT_FAILURE);
        
        }
    
    }
    
    free (blank);

}

static void write_reserved (void) {

    struct msdos_boot_sector bs;
    struct msdos_volume_info *vi = (state->size_fat == 32 ? &bs.fstype._fat32.vi : &bs.fstype._oldfat.vi);
    
    memset (&bs, 0, sizeof (bs));
    
    if (state->boot) {
    
        FILE *ifp;
        
        if ((ifp = fopen (state->boot, "rb")) == NULL) {
        
            report_at (program_name, 0, REPORT_ERROR, "unable to open %s", state->boot);
            
            fclose (ofp);
            remove (state->outfile);
            
            exit (EXIT_FAILURE);
        
        }
        
        fseek (ifp, 0, SEEK_END);
        
        if (ftell (ifp) != sizeof (bs)) {
        
            report_at (program_name, 0, REPORT_ERROR, "boot sector must be %lu bytes in size", (unsigned long) sizeof (bs));
            fclose (ifp);
            
            fclose (ofp);
            remove (state->outfile);
            
            exit (EXIT_FAILURE);
        
        }
        
        fseek (ifp, 0, SEEK_SET);
        
        if (fread (&bs, sizeof (bs), 1, ifp) != 1) {
        
            report_at (program_name, 0, REPORT_ERROR, "failed to read %s", state->boot);
            fclose (ifp);
            
            fclose (ofp);
            remove (state->outfile);
            
            exit (EXIT_FAILURE);
        
        }
        
        fclose (ifp);
    
    } else {
    
        bs.boot_jump[0] = 0xEB;
        bs.boot_jump[1] = ((state->size_fat == 32 ? (char *) &bs.fstype._fat32.boot_code : (char *) &bs.fstype._oldfat.boot_code) - (char *) &bs) - 2;
        bs.boot_jump[2] = 0x90;
        
        if (state->size_fat == 32) {
            memcpy (bs.fstype._fat32.boot_code, dummy_boot_code, sizeof (dummy_boot_code));
        } else {
            memcpy (bs.fstype._oldfat.boot_code, dummy_boot_code, sizeof (dummy_boot_code));
        }
        
        bs.boot_sign[0] = 0x55;
        bs.boot_sign[1] = 0xAA;
    
    }
    
    if (bs.boot_jump[0] != 0xEB || bs.boot_jump[1] < 0x16 || bs.boot_jump[2] != 0x90) {
        goto _write_reserved;
    }
    
    memcpy (bs.system_id, "MSWIN4.1", 8);
    
    bs.sectors_per_cluster = sectors_per_cluster;
    bs.no_fats = number_of_fats;
    bs.media_descriptor = media_descriptor;
    
    write721_to_byte_array (bs.bytes_per_sector, 512);
    write721_to_byte_array (bs.reserved_sectors, reserved_sectors);
    write721_to_byte_array (bs.root_entries, root_entries);
    write721_to_byte_array (bs.total_sectors16, total_sectors);
    write721_to_byte_array (bs.sectors_per_fat16, sectors_per_fat);
    
    if (bs.boot_jump[1] < 0x22) {
        goto _write_reserved;
    }
    
    write721_to_byte_array (bs.sectors_per_track, sectors_per_track);
    write721_to_byte_array (bs.heads_per_cylinder, heads_per_cylinder);
    write741_to_byte_array (bs.hidden_sectors, hidden_sectors);
    
    if (total_sectors > USHRT_MAX) {
    
        write721_to_byte_array (bs.total_sectors16, 0);
        write741_to_byte_array (bs.total_sectors32, total_sectors);
    
    }
    
    if (state->size_fat == 32) {
    
        if (bs.boot_jump[1] < 0x58) {
            goto _write_reserved;
        }
        
        write721_to_byte_array (bs.sectors_per_fat16, 0);
        write741_to_byte_array (bs.fstype._fat32.sectors_per_fat32, sectors_per_fat);
        
        write741_to_byte_array (bs.fstype._fat32.root_cluster, root_cluster);
        write721_to_byte_array (bs.fstype._fat32.info_sector, info_sector);
        write721_to_byte_array (bs.fstype._fat32.backup_boot, backup_boot);
        
        vi->drive_no = (media_descriptor == 0xF8 ? 0x80 : 0x00);
        vi->ext_boot_sign = 0x29;
        
        write741_to_byte_array (vi->volume_id, generate_volume_id ());
        memcpy (vi->volume_label, state->label, 11);
        memcpy (vi->fs_type, "FAT32   ", 8);
    
    } else {
    
        if (bs.boot_jump[1] < 0x3C) {
            goto _write_reserved;
        }
        
        vi->drive_no = (media_descriptor == 0xF8 ? 0x80 : 0x00);
        vi->ext_boot_sign = 0x29;
        
        write741_to_byte_array (vi->volume_id, generate_volume_id ());
        memcpy (vi->volume_label, state->label, 11);
        
        if (state->size_fat == 12) {
            memcpy (vi->fs_type, "FAT12   ", 8);
        } else {
            memcpy (vi->fs_type, "FAT16   ", 8);
        }
    
    }

_write_reserved:

    seekto (0);
    
    if (fwrite (&bs, sizeof (bs), 1, ofp) != 1) {
    
        report_at (program_name, 0, REPORT_ERROR, "Failed whilst writing %s", state->outfile);
        
        fclose (ofp);
        remove (state->outfile);
        
        exit (EXIT_FAILURE);
    
    }
    
    if (state->size_fat == 32) {
    
        if (info_sector) {
        
            struct fat32_fsinfo *info;
            unsigned char *buffer;
            
            if (seekto (info_sector * 512)) {
            
                report_at (program_name, 0, REPORT_ERROR, "Failed whilst seeking %s", state->outfile);
                
                fclose (ofp);
                remove (state->outfile);
                
                exit (EXIT_FAILURE);
            
            }
            
            if (!(buffer = (unsigned char *) malloc (512))) {
            
                report_at (program_name, 0, REPORT_ERROR, "Failed to allocate memory");
                
                fclose (ofp);
                remove (state->outfile);
                
                exit (EXIT_FAILURE);
            
            }
            
            memset (buffer, 0, 512);
            
            /** fsinfo structure is at offset 0x1e0 in info sector by observation. */
            info = (struct fat32_fsinfo *) (buffer + 0x1e0);
            
            /** Info sector magic. */
            buffer[0] = 'R';
            buffer[1] = 'R';
            buffer[2] = 'a';
            buffer[3] = 'A';
            
            /** Magic for fsinfo structure. */
            write741_to_byte_array (info->signature, 0x61417272);
            
            /** We've allocated cluster 2 for the root directory. */
            write741_to_byte_array (info->free_clusters, cluster_count - 1);
            write741_to_byte_array (info->next_cluster, 2);
            
            /** Info sector also must have boot sign. */
            write721_to_byte_array (buffer + 0x1fe, 0xAA55);
            
            if (fwrite (buffer, 512, 1, ofp) != 1) {
            
                report_at (program_name, 0, REPORT_ERROR, "Failed whilst writing info sector");
                free (buffer);
                
                fclose (ofp);
                remove (state->outfile);
                
                exit (EXIT_FAILURE);
            
            }
            
            free (buffer);
        
        }
        
        if (backup_boot) {
        
            if (seekto (backup_boot * 512) || fwrite (&bs, sizeof (bs), 1, ofp) != 1) {
            
                report_at (program_name, 0, REPORT_ERROR, "Failed whilst writing info sector");
                
                fclose (ofp);
                remove (state->outfile);
                
                exit (EXIT_FAILURE);
            
            }
        
        }
    
    }

}

int main (int argc, char **argv) {

    if (argc && *argv) {
    
        char *p;
        program_name = *argv;
        
        if ((p = strrchr (program_name, '/'))) {
            program_name = (p + 1);
        }
    
    }
    
    state = xmalloc (sizeof (*state));
    parse_args (&argc, &argv, 1);
    
    if (!state->outfile) {
    
        report_at (program_name, 0, REPORT_ERROR, "no outfile file provided");
        return EXIT_FAILURE;
    
    }
    
    image_size = state->blocks * 1024;
    image_size += state->offset * 512;
    
    if ((ofp = fopen (state->outfile, "r+b")) == NULL) {
    
        size_t len;
        void *zero;
        
        if ((ofp = fopen (state->outfile, "w+b")) == NULL) {
        
            report_at (program_name, 0, REPORT_ERROR, "failed to open '%s' for writing", state->outfile);
            return EXIT_FAILURE;
        
        }
        
        len = image_size;
        zero = xmalloc (512);
        
        while (len > 0) {
        
            if (fwrite (zero, 512, 1, ofp) != 1) {
            
                report_at (program_name, 0, REPORT_ERROR, "failed whilst writing '%s'", state->outfile);
                fclose (ofp);
                
                free (zero);
                remove (state->outfile);
                
                return EXIT_FAILURE;
            
            }
            
            len -= 512;
        
        }
        
        free (zero);
    
    }
    
    if (image_size == 0) {
    
        fseek (ofp, 0, SEEK_END);
        image_size = ftell (ofp);
    
    }
    
    seekto (0);
    
    if (state->offset * 512 > image_size) {
    
        report_at (program_name, 0, REPORT_ERROR, "size (%lu) of %s is less than the requested offset (%lu)", image_size, state->outfile, state->offset * 512);
        
        fclose (ofp);
        remove (state->outfile);
        
        return EXIT_FAILURE;
    
    }
    
    image_size -= state->offset * 512;
    
    if (state->blocks) {
    
        if (state->blocks * 1024 > image_size) {
        
            report_at (program_name, 0, REPORT_ERROR, "size (%lu) of %s is less than the requested size (%lu)", image_size, state->outfile, state->blocks * 1024);
            
            fclose (ofp);
            remove (state->outfile);
            
            return EXIT_FAILURE;
        
        }
        
        image_size = state->blocks * 1024;
    
    }
    
    orphaned_sectors = (image_size % 1024) / 512;
    
    establish_bpb ();
    wipe_target ();
    
    write_reserved ();
    
    if (set_fat_entry (0, 0xFFFFFF00 | media_descriptor) < 0 || set_fat_entry (1, 0xFFFFFFFF) < 0) {
    
        report_at (program_name, 0, REPORT_ERROR, "Failed whilst setting FAT entry");
        
        fclose (ofp);
        remove (state->outfile);
        
        return EXIT_FAILURE;
    
    }
    
    if (state->size_fat == 32) {
    
        if (set_fat_entry (2, 0x0FFFFFF8) < 0) {
        
            report_at (program_name, 0, REPORT_ERROR, "Failed whilst setting FAT entry");
            
            fclose (ofp);
            remove (state->outfile);
            
            return EXIT_FAILURE;
        
        }
    
    }
    
    if (memcmp (state->label, "NO NAME    ", 11) != 0) {
        add_volume_label ();
    }
    
    fclose (ofp);
    return EXIT_SUCCESS;

}

#endif /* mkfs.c  --------------------- */
#ifndef report_c

/******************************************************************************
 * @file            report.c
 *****************************************************************************/
#include    <stdarg.h>
#include    <stdio.h>

#if 0
#include    "report.h"
#endif
#ifndef     __PDOS__
#if     defined (_WIN32)
# include   <windows.h>
static int OriginalConsoleColor = -1;
#endif

static void reset_console_color (void) {

#if     defined (_WIN32)

    HANDLE hStdError = GetStdHandle (STD_ERROR_HANDLE);
    
    if (OriginalConsoleColor == -1) { return; }
    
    SetConsoleTextAttribute (hStdError, OriginalConsoleColor);
    OriginalConsoleColor = -1;

#else

    fprintf (stderr, "\033[0m");

#endif

}

static void set_console_color (int color) {

#if     defined (_WIN32)

    HANDLE hStdError = GetStdHandle (STD_ERROR_HANDLE);
    WORD wColor;
    
    if (OriginalConsoleColor == -1) {
    
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        
        if (!GetConsoleScreenBufferInfo (hStdError, &csbi)) {
            return;
        }
        
        OriginalConsoleColor = csbi.wAttributes;
    
    }
    
    wColor = (OriginalConsoleColor & 0xF0) + (color & 0xF);
    SetConsoleTextAttribute (hStdError, wColor);

#else

    fprintf (stderr, "\033[%dm", color);

#endif

}
#endif

void report_at (const char *filename, unsigned long line_number, int type, const char *fmt, ...) {

    va_list ap;
    
    if (filename) {
    
        if (line_number == 0) {
            fprintf (stderr, "%s: ", filename);
        } else {
            fprintf (stderr, "%s:", filename);
        }
    
    }
    
    if (line_number > 0) {
        fprintf (stderr, "%lu: ", line_number);
    }
    
    if (type == REPORT_ERROR || type == REPORT_FATAL_ERROR) {
    
#ifndef     __PDOS__
        set_console_color (COLOR_ERROR);
#endif
        
        if (type == REPORT_ERROR) {
            fprintf (stderr, "error:");
        } else {
            fprintf (stderr, "fatal error:");
        }
    
    } else if (type == REPORT_INTERNAL_ERROR) {
    
#ifndef     __PDOS__
        set_console_color (COLOR_INTERNAL_ERROR);
#endif
        
        fprintf (stderr, "internal error:");
    
    } else if (type == REPORT_WARNING) {
    
        #ifndef     __PDOS__
        set_console_color (COLOR_INTERNAL_ERROR);
#endif
        
        fprintf (stderr, "warning:");
    
    }
    
#ifndef     __PDOS__
    reset_console_color ();
#endif
    
    fprintf (stderr, " ");
    
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    
    fprintf (stderr, "\n");

}

#endif /* report.c --------------------- */
#ifndef write7x_c

/******************************************************************************
 * @file            write7x.c
 *****************************************************************************/
#if 0
#include    "write7x.h"
#endif

void write721_to_byte_array (unsigned char *dest, unsigned short val) {

    dest[0] = (val & 0xFF);
    dest[1] = (val >> 8) & 0xFF;

}

void write741_to_byte_array (unsigned char *dest, unsigned int val) {

    dest[0] = (val & 0xFF);
    dest[1] = (val >> 8) & 0xFF;
    dest[2] = (val >> 16) & 0xFF;
    dest[3] = (val >> 24) & 0xFF;

}

#endif /* write7x.c  --------------------- */
