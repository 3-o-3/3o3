
/*
12.06.2024 downloaded form https://github.com/robertapengelly/parted
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
#endif /* LICENSE ------------------  */

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
void print_help (int exitval);

#endif      /* _LIB_H */
#endif /* lib.h ------------------  */

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
#endif /* report.h ------------------   */


#ifndef write7x_h
/******************************************************************************
 * @file            write7x.h
 *****************************************************************************/
#ifndef     _WRITE7X_H
#define     _WRITE7X_H

void write721_to_byte_array (unsigned char *dest, unsigned short val, int little_endian);
void write741_to_byte_array (unsigned char *dest, unsigned int val, int little_endian);
void write781_to_byte_array (unsigned char *dest, unsigned long val, int little_endian);

#endif      /* _WRITE7X_H */
#endif /* write7x.h ------------------  */

#ifndef parted_h
/******************************************************************************
 * @file            parted.h
 *****************************************************************************/
#ifndef     _PARTED_H
#define     _PARTED_H

#include    <stddef.h>

struct part_info {

    int status, type;
    size_t start, end;

};

struct parted_state {

    const char *boot, *outfile;
    
    struct part_info **parts;
    size_t nb_parts;
    
    size_t image_size;

};

extern struct parted_state *state;
extern const char *program_name;

#endif      /* _PARTED_H */
#endif /* parted.h ------------------  */

#ifndef parted_c

/******************************************************************************
 * @file            parted.c
 *****************************************************************************/
#include    <limits.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <time.h>

#if 0
#include    "lib.h"
#include    "parted.h"
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

static int cylinders = 0;
static int heads_per_cylinder = 0;
static int sectors_per_track = 0;

struct parted_state *state = 0;
const char *program_name = 0;

struct part {

    unsigned char status;
    unsigned char start_chs[3];
    unsigned char type;
    unsigned char end_chs[3];
    unsigned char start_lba[4];
    unsigned char end_lba[4];

};

struct mbr {

    unsigned char boot_code[440];
    unsigned char signature[4];
    unsigned char copy_protected[2];
    
    struct part parts[4];
    unsigned char boot_sig[2];

};

struct footer {

    unsigned char cookie[8];
    unsigned char features[4];
    
    struct {
    
        unsigned char major[2];
        unsigned char minor[2];
    
    } version;
    
    unsigned char next_offset[8];
    unsigned char modified_time[4];
    unsigned char creator_name[4];
    
    struct {
    
        unsigned char major[2];
        unsigned char minor[2];
    
    } creator_version;
    
    unsigned char creator_host[4];
    unsigned char disk_size[8];
    unsigned char data_size[8];
    
    struct {
    
        unsigned char cylinders[2];
        unsigned char heads_per_cyl;
        unsigned char secs_per_track;
    
    } disk_geom;
    
    unsigned char disk_type[4];
    unsigned char checksum[4];
    unsigned char identifier[16];
    unsigned char saved_state;
    unsigned char reserved[427];

};

static void calculate_geometry (void) {

    int cylinder_times_heads;
    long sectors = state->image_size / 512;
    
    if (sectors > (long) 65535 * 16 * 255) {
        sectors = (long) 65535 * 16 * 255;
    }
    
    if (sectors > (long) 65535 * 16 * 63) {
    
        heads_per_cylinder = 16;
        sectors_per_track = 63;
        
        cylinder_times_heads = sectors / sectors_per_track;
    
    } else {
    
        sectors_per_track = 17;
        cylinder_times_heads = sectors / sectors_per_track;
        
        heads_per_cylinder = (cylinder_times_heads + 1023) >> 10;
        
        if (heads_per_cylinder < 4) {
            heads_per_cylinder = 4;
        }
        
        if (cylinder_times_heads >= heads_per_cylinder << 10 || heads_per_cylinder > 16) {
        
            sectors_per_track = 31;
            heads_per_cylinder = 16;
            
            cylinder_times_heads = sectors / sectors_per_track;
        
        }
        
        if (cylinder_times_heads >= heads_per_cylinder << 10) {
        
            sectors_per_track = 63;
            heads_per_cylinder = 16;
            
            cylinder_times_heads = sectors / sectors_per_track;
        
        }
    
    }
    
    cylinders = cylinder_times_heads / heads_per_cylinder;

}

/** Generate a unique volume id. */
static unsigned int generate_signature (void) {

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

#ifdef _WIN32
int getpid() {
    return (int)&getpid;
}
#endif

static void get_random_bytes (void *buf, size_t nbytes) {

    unsigned char *cp = (unsigned char *) buf;
    unsigned int seed;
    
    size_t i;
    
#if     defined (__PDOS__)
    seed = time (NULL);
#elif   defined (__GNUC__)

    struct timeval now;
    seed = (getpid () << 16);
    
    if (gettimeofday (&now, 0) != 0 || now.tv_sec == (time_t) -1 || now.tv_sec < 0) {
        seed = seed ^ now.tv_sec ^ now.tv_usec;
    } else {
        seed ^= time (NULL);
    }

#else

    seed = (getpid () << 16);
    seed ^= time (NULL);

#endif
    
    srand (seed);
    
    for (i = 0; i < nbytes; i++) {
        *cp++ ^= (rand () >> 7) % (CHAR_MAX + 1);
    }

}

static unsigned int generate_checksum (unsigned char *data, size_t len) {

    unsigned int sum = 0;
    int i = 0;
    
    size_t j;
    
    for (j = 0; j < len; j++) {
        sum += data[i++];
    }
    
    return sum ^ 0xffffffff;

}

int main (int argc, char **argv) {

    size_t flen;
    FILE *ofp;
    
    struct footer footer;
    struct mbr mbr;
    
    size_t p, sector = 1;
    
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
    
    if (!state->outfile || state->nb_parts == 0) {
        print_help (EXIT_FAILURE);
    }
    
    memset (&mbr, 0, sizeof (mbr));
    
    if (state->boot) {
    
        FILE *ifp;
        
        if ((ifp = fopen (state->boot, "rb")) == NULL) {
        
            report_at (program_name, 0, REPORT_ERROR, "failed to open '%s' for reading", state->boot);
            return EXIT_FAILURE;
        
        }
        
        fseek (ifp, 0, SEEK_END);
        
        if (ftell (ifp) != 440) {
        
            report_at (program_name, 0, REPORT_ERROR, "size of %s is not equal to 440 bytes", state->boot);
            fclose (ifp);
            
            return EXIT_FAILURE;
        
        }
        
        fseek (ifp, 0, SEEK_SET);
        
        if (fread (&mbr, 440, 1, ifp) != 1) {
        
            report_at (program_name, 0, REPORT_ERROR, "failed whilst reading '%s'", state->boot);
            fclose (ifp);
            
            return EXIT_FAILURE;
        
        }
        
        fclose (ifp);
    
    }
    
    write721_to_byte_array (mbr.boot_sig, 0xAA55, 1);
    write741_to_byte_array (mbr.signature, generate_signature (), 1);
    
    calculate_geometry ();
    
    for (p = 0; p < state->nb_parts; ++p) {
    
        struct part_info *info = state->parts[p];
        unsigned cyl, head, sect, temp;
        
        if (p >= 4) {
        
            report_at (program_name, 0, REPORT_ERROR, "too many partitions provided");
            return EXIT_FAILURE;
        
        }
        
        mbr.parts[p].status = info->status;
        mbr.parts[p].type = info->type;
        
        if (info->start < sector) {
        
            report_at (program_name, 0, REPORT_ERROR, "overlapping partitions");
            return EXIT_FAILURE;
        
        }
        
        sector = info->start;
        
        if (sector * 512 > state->image_size) {
        
            report_at (program_name, 0, REPORT_ERROR, "partition exceeds image size");
            return EXIT_FAILURE;
        
        }
        
        cyl = sector / (heads_per_cylinder * sectors_per_track);
        temp = sector % (heads_per_cylinder * sectors_per_track);
        
        head = temp / sectors_per_track;
        sect = temp % sectors_per_track + 1;
        
        mbr.parts[p].start_chs[0] = head;
        mbr.parts[p].start_chs[1] = sect;
        mbr.parts[p].start_chs[2] = cyl;
        
        write741_to_byte_array (mbr.parts[p].start_lba, info->start, 1);
        
        if (info->end < sector) {
        
            report_at (program_name, 0, REPORT_ERROR, "overlapping partitions");
            return EXIT_FAILURE;
        
        }
        
        sector = info->end;
        
        if (sector * 512 > state->image_size) {
        
            report_at (program_name, 0, REPORT_ERROR, "partition exceeds image size");
            return EXIT_FAILURE;
        
        }
        
        cyl = sector / (heads_per_cylinder * sectors_per_track);
        temp = sector % (heads_per_cylinder * sectors_per_track);
        
        head = temp / sectors_per_track;
        sect = temp % sectors_per_track + 1;
        
        mbr.parts[p].end_chs[0] = head;
        mbr.parts[p].end_chs[1] = sect;
        mbr.parts[p].end_chs[2] = cyl;
        
        write741_to_byte_array (mbr.parts[p].end_lba, info->end, 1);
    
    }
    
    state->image_size += 512;
    state->image_size += sizeof (footer);
    
    if ((ofp = fopen (state->outfile, "r+b")) == NULL) {
    
        size_t len;
        void *zero;
        
        if ((ofp = fopen (state->outfile, "wb")) == NULL) {
        
            report_at (program_name, 0, REPORT_ERROR, "failed to open '%s' for writing", state->outfile);
            return EXIT_FAILURE;
        
        }
        
        len = state->image_size;
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
    
    fseek (ofp, 0, SEEK_END);
    flen = (size_t) ftell (ofp);
    
    if (flen < state->image_size) {
    
        report_at (program_name, 0, REPORT_ERROR, "file size mismatch (%lu vs %lu)", flen, state->image_size);
        fclose (ofp);
        
        remove (state->outfile);
        return EXIT_FAILURE;
    
    }
    
    fseek (ofp, 0, SEEK_SET);
    
    if (fwrite (&mbr, sizeof (mbr), 1, ofp) != 1) {
    
        report_at (program_name, 0, REPORT_ERROR, "failed whilst writing the master boot record");
        
        fclose (ofp);
        remove (state->outfile);
        
        return EXIT_FAILURE;
    
    }
    
    memset (&footer, 0, sizeof (footer));
    
    memcpy (footer.cookie, "conectix", 8);
    memcpy (footer.creator_host, "Wi2k", 4);
    memcpy (footer.creator_name, "win ", 4);
    
    write721_to_byte_array (footer.version.major, 1, 0);
    write721_to_byte_array (footer.version.minor, 0, 0);
    write721_to_byte_array (footer.creator_version.major, 6, 0);
    write721_to_byte_array (footer.creator_version.minor, 1, 0);
    
    write741_to_byte_array (footer.features, 2, 0);
    write741_to_byte_array (footer.disk_type, 2, 0);
    
    memset (footer.next_offset, 0xff, 8);
    
    write781_to_byte_array (footer.disk_size, flen - sizeof (footer), 0);
    write781_to_byte_array (footer.data_size, flen - sizeof (footer), 0);
    
    write721_to_byte_array (footer.disk_geom.cylinders, cylinders, 0);
    footer.disk_geom.heads_per_cyl = heads_per_cylinder;
    footer.disk_geom.secs_per_track = sectors_per_track;
    
    get_random_bytes (footer.identifier, sizeof (footer.identifier));
    
    write741_to_byte_array (footer.modified_time, time (NULL) - 946684800, 0);
    write741_to_byte_array (footer.checksum, generate_checksum ((unsigned char *) &footer, sizeof (footer)), 0);
    
    if (fseek (ofp, state->image_size - sizeof (footer), SEEK_SET)) {
    
        report_at (program_name, 0, REPORT_ERROR, "failed whilst seeking '%s'", state->outfile);
        
        fclose (ofp);
        remove (state->outfile);
        
        return EXIT_FAILURE;
    
    }
    
    if (fwrite (&footer, sizeof (footer), 1, ofp) != 1) {
    
        report_at (program_name, 0, REPORT_ERROR, "failed whilst writing footer");
        
        fclose (ofp);
        remove (state->outfile);
        
        return EXIT_FAILURE;
    
    }
    
    fclose (ofp);
    return EXIT_SUCCESS;

}

#endif /* parted.c ------------------  */

#ifndef lib_c

/******************************************************************************
 * @file            lib.c
 *****************************************************************************/
#include    <ctype.h>
#include    <errno.h>
#include    <stddef.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#if 0
#include    "lib.h"
#include    "parted.h"
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
    OPTION_BOOT,
    OPTION_HELP,
    OPTION_LABEL,
    OPTION_PART

};

static struct option opts[] = {

    { "-boot",      OPTION_BOOT,        OPTION_HAS_ARG  },
    { "-help",      OPTION_HELP,        OPTION_NO_ARG   },
    { 0,            0,                  0               }

};

static struct option opts1[] = {

    { "mklabel",    OPTION_LABEL,       OPTION_HAS_ARG  },
    { "mkpart",     OPTION_PART,        OPTION_HAS_ARG  },
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

static char *skip_whitespace (char *p) {

    while (*p && *p == ' ') {
        ++p;
    }
    
    return p;

}

static char *trim (char *p) {

    size_t len = strlen (p);
    size_t i;
    
    for (i = len; i > 0; --i) {
    
        if (p[i] == ' ') {
            p[i] = '\0';
        }
    
    }
    
    if (p[i] == ' ') {
        p[i] = '\0';
    }
    
    return p;

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
        print_help (EXIT_SUCCESS);
    }
    
    while (optind < argc) {
    
        r = argv[optind++];
        
        if (r[0] != '-' || r[1] == '\0') {
        
            for (popt = opts1; popt->name; ++popt) {
            
                const char *p1 = popt->name;
                
                if (xstrcasecmp (p1, r) != 0) {
                    continue;
                }
                
                if (popt->flags & OPTION_HAS_ARG) {
                
                    if (optind >= argc) {
                    
                        report_at (program_name, 0, REPORT_ERROR, "argument to '%s' is missing", r);
                        exit (EXIT_FAILURE);
                    
                    }
                    
                    optarg = argv[optind++];
                    goto have_opt;
                
                }
            
            }
            
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
        
    have_opt:
        
        switch (popt->index) {
        
            case OPTION_BOOT: {
            
                state->boot = xstrdup (optarg);
                break;
            
            }
            
            case OPTION_HELP: {
            
                print_help (EXIT_SUCCESS);
                break;
            
            }
            
            case OPTION_LABEL: {
            
                /*report_at (__FILE__, __LINE__, REPORT_INTERNAL_ERROR, "label: %s", optarg);*/
                break;
            
            }
            
            case OPTION_PART: {
            
                long conversion, size;
                char *temp;
                
                struct part_info *part = xmalloc (sizeof (*part));
                
                char *p1 = skip_whitespace ((char *) optarg);
                char *p2 = p1;
                
                while (*p2 && *p2 != ',') {
                    p2++;
                }
                
                if (*p2 == ',') {
                    *p2++ = '\0';
                }
                
                p1 = trim (p1);
                
                errno = 0;
                conversion = strtol (p1, &temp, 0);
                
                if (!*p1 || isspace ((int) *p1) || *temp || errno) {
                
                    report_at (program_name, 0, REPORT_ERROR, "failed to get partition status");
                    exit (EXIT_FAILURE);
                
                }
                
                if (conversion != 0 && conversion != 128) {
                
                    report_at (program_name, 0, REPORT_ERROR, "partition status must be either 0 or 128");
                    exit (EXIT_FAILURE);
                
                }
                
                part->status = conversion;
                
                p2 = skip_whitespace (p2);
                p1 = p2;
                
                while (*p2 && *p2 != ',') {
                    p2++;
                }
                
                if (*p2 == ',') {
                    *p2++ = '\0';
                }
                
                p1 = trim (p1);
                
                errno = 0;
                conversion = strtol (p1, &temp, 0);
                
                if (!*p1 || isspace ((int) *p1) || *temp || errno) {
                
                    report_at (program_name, 0, REPORT_ERROR, "failed to get partition type");
                    exit (EXIT_FAILURE);
                
                }
                
                switch (conversion) {
                
                    case 0x01:
                    case 0x04:
                    case 0x06:
                    case 0x0B:
                    case 0x0C:
                    case 0x0E:
                    
                        break;
                    
                    default:
                    
                        report_at (program_name, 0, REPORT_ERROR, "unsupported partition type provided");
                        exit (EXIT_FAILURE);
                
                }
                
                part->type = conversion;
                
                p2 = skip_whitespace (p2);
                p1 = p2;
                
                while (*p2 && *p2 != ',') {
                    p2++;
                }
                
                if (*p2 == ',') {
                    *p2++ = '\0';
                }
                
                p1 = trim (p1);
                
                errno = 0;
                conversion = strtol (p1, &temp, 0);
                
                if (!*p1 || isspace ((int) *p1) || *temp || errno) {
                
                    report_at (program_name, 0, REPORT_ERROR, "failed to get partition start");
                    exit (EXIT_FAILURE);
                
                }
                
                if (conversion < 0) {
                
                    report_at (program_name, 0, REPORT_ERROR, "partition start must be greater than 0");
                    exit (EXIT_FAILURE);
                
                }
                
                part->start = (size_t) conversion;
                
                p2 = skip_whitespace (p2);
                p1 = p2;
                
                while (*p2 && *p2 != ',') {
                    p2++;
                }
                
                if (*p2 == ',') {
                    *p2++ = '\0';
                }
                
                p1 = trim (p1);
                
                errno = 0;
                conversion = strtol (p1, &temp, 0);
                
                if (!*p1 || isspace ((int) *p1) || *temp || errno) {
                
                    report_at (program_name, 0, REPORT_ERROR, "failed to get partition end");
                    exit (EXIT_FAILURE);
                
                }
                
                if (conversion < 0) {
                
                    report_at (program_name, 0, REPORT_ERROR, "partition end must be greater than 0");
                    exit (EXIT_FAILURE);
                
                }
                
                part->end = (size_t) conversion;
                
                if (part->end < part->start) {
                
                    report_at (program_name, 0, REPORT_ERROR, "partition end must be greater than partition start");
                    exit (EXIT_FAILURE);
                
                }
                
                size = (part->start + part->end) * 512;
                
                if (state->image_size < (size_t) size) {
                    state->image_size = (size_t) size;
                }
                
                dynarray_add (&state->parts, &state->nb_parts, part);
                break;
            
            }
            
            default: {
            
                report_at (program_name, 0, REPORT_ERROR, "unsupported option '%s'", r);
                exit (EXIT_FAILURE);
            
            }
        
        }
    
    }

}

void print_help (int exitval) {

    if (!program_name) {
        goto _exit;
    }
    
    fprintf (stderr, "Usage: %s [options] [commands] file\n\n", program_name);
    fprintf (stderr, "Options:\n\n");
    fprintf (stderr, "    --boot FILE           Write FILE to start of image\n");
    fprintf (stderr, "    --help                Print this help information\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "Commands:\n\n");
    fprintf (stderr, "    mkpart status,type,start,end  Make a partition\n");
    
_exit:
    
    exit (exitval);

}

#endif /* lib.c ------------------  */

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

#endif /* report.c ------------------  */

#ifndef write7x_c

/******************************************************************************
 * @file            write7x.c
 *****************************************************************************/
#include    <limits.h>

#if 0
#include    "write7x.h"
#endif
void write721_to_byte_array (unsigned char *dest, unsigned short val, int little_endian) {

    if (little_endian) {
    
        dest[0] = (val & 0xFF);
        dest[1] = (val >> 8) & 0xFF;
    
    } else {
    
        dest[1] = (val & 0xFF);
        dest[0] = (val >> 8) & 0xFF;
    
    }

}

void write741_to_byte_array (unsigned char *dest, unsigned int val, int little_endian) {

    if (little_endian) {
    
        dest[0] = (val & 0xFF);
        dest[1] = (val >> 8) & 0xFF;
        dest[2] = (val >> 16) & 0xFF;
        dest[3] = (val >> 24) & 0xFF;
    
    } else {
    
        dest[3] = (val & 0xFF);
        dest[2] = (val >> 8) & 0xFF;
        dest[1] = (val >> 16) & 0xFF;
        dest[0] = (val >> 24) & 0xFF;
    
    }

}

void write781_to_byte_array (unsigned char *dest, unsigned long val, int little_endian) {

    if (little_endian) {
    
        dest[0] = (val & 0xFF);
        dest[1] = (val >> 8) & 0xFF;
        dest[2] = (val >> 16) & 0xFF;
        dest[3] = (val >> 24) & 0xFF;
        
#if     ULONG_MAX > 4294967295UL
        
        dest[4] = (val >> 32) & 0xFF;
        dest[5] = (val >> 40) & 0xFF;
        dest[6] = (val >> 48) & 0xFF;
        dest[7] = (val >> 56) & 0xFF;
        
#else
        
        dest[4] = 0;
        dest[5] = 0;
        dest[6] = 0;
        dest[7] = 0;
        
#endif
    
    } else {
    
        dest[7] = (val & 0xFF);
        dest[6] = (val >> 8) & 0xFF;
        dest[5] = (val >> 16) & 0xFF;
        dest[4] = (val >> 24) & 0xFF;
        
#if     ULONG_MAX > 4294967295UL
        
        dest[3] = (val >> 32) & 0xFF;
        dest[2] = (val >> 40) & 0xFF;
        dest[1] = (val >> 48) & 0xFF;
        dest[0] = (val >> 56) & 0xFF;
     
#else
        
        dest[3] = 0;
        dest[2] = 0;
        dest[1] = 0;
        dest[0] = 0;
        
#endif
    
    }

}

#endif /* write7x.c ------------------  */

#ifndef _
#endif /* ------------------  */