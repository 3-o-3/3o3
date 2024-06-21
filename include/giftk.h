/*
 *
 *          giftk - GIF drawing toolkit
 *
 *         PUBLIC DOMAIN MMXXIV by Jean-Marc Lienher
 *	    The authors disclam copyright to this software.
 *
 * Based on :
 * stb_image - v2.30 - public domain image loader - http://nothings.org/stb
 *                                 no warranty implied; use at your own risk
 *
 * https://github.com/nothings/stb/blob/master/stb_image.h
 * 
 * stb_truetype.h - v1.26 - public domain
 * authored from 2009-2021 by Sean Barrett / RAD Game Tools
 * https://github.com/nothings/stb/blob/master/stb_truetype.h
 *
 *
 * LICENSE
 *
 *  See end of file for license information.
 *
 */

/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef GIFTK_H_
#define GIFTK_H_ 1

#ifdef __cplusplus
extern "C" {
#endif


    // #define your own (u)giftk_int8/16/32 before including to override this
#ifndef giftk_uint8
    typedef unsigned char   giftk_uint8;
    typedef signed   char   giftk_int8;
    typedef unsigned short  giftk_uint16;
    typedef signed   short  giftk_int16;
    typedef unsigned int    giftk_uint32;
    typedef signed   int    giftk_int32;
#endif

    typedef char giftk__check_size32[sizeof(giftk_int32) == 4 ? 1 : -1];
    typedef char giftk__check_size16[sizeof(giftk_int16) == 2 ? 1 : -1];

    // e.g. #define your own GIFTK_ifloor/GIFTK_iceil() to avoid math.h
#ifndef GIFTK_ifloor
#include <math.h>
#define GIFTK_ifloor(x)   ((int) floor(x))
#define GIFTK_iceil(x)    ((int) ceil(x))
#endif

#ifndef GIFTK_sqrt
#include <math.h>
#define GIFTK_sqrt(x)      sqrt(x)
#define GIFTK_pow(x,y)     pow(x,y)
#endif

#ifndef GIFTK_fmod
#include <math.h>
#define GIFTK_fmod(x,y)    fmod(x,y)
#endif

#ifndef GIFTK_cos
#include <math.h>
#define GIFTK_cos(x)       cos(x)
#define GIFTK_acos(x)      acos(x)
#endif

#ifndef GIFTK_fabs
#include <math.h>
#define GIFTK_fabs(x)      fabs(x)
#endif

// #define your own functions "GIFTK_malloc" / "GIFTK_free" to avoid malloc.h
#ifndef GIFTK_malloc
#include <stdlib.h>
#define GIFTK_malloc(x,u)  ((void)(u),malloc(x))
#define GIFTK_free(x,u)    ((void)(u),free(x))
#endif

#ifndef GIFTK_assert
#include <assert.h>
#define GIFTK_assert(x)    assert(x)
#endif

#ifndef GIFTK_strlen
#include <string.h>
#define GIFTK_strlen(x)    strlen(x)
#endif

#ifndef GIFTK_memcpy
#include <string.h>
#define GIFTK_memcpy       memcpy
#define GIFTK_memset       memset
#endif


#define GIFTK_API extern
#ifdef GIFTK_STATIC
#define GIFTK_DEF static
#else
#define GIFTK_DEF extern
#endif

struct giftk__context;
struct nk_color;

GIFTK_API struct giftk__context* giftk__init(char const* file);
GIFTK_API void giftk__shutdown(struct giftk__context* gif);
GIFTK_API void giftk__resize(struct giftk__context* gif, void* fb, const unsigned int w, const unsigned int h, const unsigned int pitch);


// private structure
typedef struct
{
    unsigned char* data;
    int cursor;
    int size;
} giftk__buf;

//////////////////////////////////////////////////////////////////////////////
//
// GLYPH SHAPES (you probably don't need these, but they have to go before
// the bitmaps for C declaration-order reasons)
//

#ifndef GIFTK_vmove // you can predefine these to use different values (but why?)
enum {
    GIFTK_vmove = 1,
    GIFTK_vline,
    GIFTK_vcurve,
    GIFTK_vcubic
};
#endif

#ifndef giftk__vertex // you can predefine this to use different values
// (we share this with other code at RAD)
#define giftk__vertex_type short // can't use giftk_int16 because that's not visible in the header file
typedef struct
{
    giftk__vertex_type x, y, cx, cy, cx1, cy1;
    unsigned char type, padding;
} giftk__vertex;
#endif


typedef struct
{
    int w, h, stride;
    unsigned char* pixels;
} giftk__bitmap;

// rasterize a shape with quadratic beziers into a bitmap
GIFTK_DEF void giftk__render(struct giftk__context* result,        // 1-channel bitmap to draw into
    float flatness_in_pixels,     // allowable error of curve in pixels
    giftk__vertex* vertices,       // array of vertices defining shape
    int num_verts,                // number of vertices in above array
    float scale_x, float scale_y, // scale applied to input vertices
    float shift_x, float shift_y, // translation applied to input vertices
    int x_off, int y_off,         // another translation applied to input
    int invert,                   // if non-zero, vertically flip shape
    void* userdata,              // context for to GIFTK_MALLOC
    struct nk_color c);

#ifdef __cplusplus
}
#endif
#endif /* GIFTK_H_ */

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef GIFTK_IMPLEMENTATION

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define giftk__inline

#define GIFTK_ASSERT(a)
#define GIFTK_FREE(a) free(a)
#define GIFTK_MALLOC(a) malloc(a)
#define GIFTK_REALLOC(p, newsz) realloc(p, newsz)
#define GIFTK_REALLOC_SIZED(p, oldsz, newsz) GIFTK_REALLOC(p, newsz)
#define giftk_uc unsigned char
#define giftk_int16 short
#define giftk_uint16 unsigned short
#define giftk_int32 int
#define giftk_uint32 unsigned int
#define GIFTK_MAX_DIMENSIONS (1 << 24)

#ifdef _MSC_VER
#define GIFTK_NOTUSED(v) (void)(v)
#else
#define GIFTK_NOTUSED(v) (void)sizeof(v)
#endif

typedef struct
{
    int (*read)(void* user, char* data, int size); // fill 'data' with 'size' bytes.  return number of bytes actually read
    void (*skip)(void* user, int n); // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
    int (*eof)(void* user); // returns nonzero if we are at end of file/data
} giftk__io_callbacks;

typedef struct giftk__context {
    unsigned int img_x, img_y, img_z;
    int img_n, img_out_n;

    unsigned char* output;
    unsigned char* origin;
    unsigned int current_z;
    int* delays;

    giftk__io_callbacks io;
    void* io_user_data;

    int read_from_callbacks;
    int buflen;
    unsigned char buffer_start[128];
    int callback_already_read;

    unsigned char *img_buffer, *img_buffer_end;
    unsigned char *img_buffer_original, *img_buffer_original_end;

    char file[512];
} giftk__context;



typedef struct giftk__result_info {
    int bits_per_channel;
    int num_channels;
    int channel_order;
} giftk__result_info;

static int giftk__vertically_flip_on_load_global = 0;
#define giftk__vertically_flip_on_load giftk__vertically_flip_on_load_global

static int giftk__gif_test(giftk__context* s);
static void* giftk__gif_load(giftk__context* s, int* x, int* y, int* comp, int req_comp, giftk__result_info* ri);
static void* giftk__load_gif_main(giftk__context* s, int** delays, int* x, int* y, int* z, int* comp, int req_comp);
static int giftk__gif_info(giftk__context* s, int* x, int* y, int* comp);

static void giftk__refill_buffer(giftk__context* s);

const char* giftk__g_failure_reason;

const char* giftk__failure_reason(void)
{
    return giftk__g_failure_reason;
}

static int giftk__err(const char* str)
{
    giftk__g_failure_reason = str;
    return 0;
}

static void* giftk__malloc(os_size size)
{
    return GIFTK_MALLOC(size);
}

static void giftk__rewind(giftk__context* s)
{
    // conceptually rewind SHOULD rewind to the beginning of the stream,
    // but we just rewind to the beginning of the initial buffer, because
    // we only use it after doing 'test', which only ever looks at at most 92 bytes
    s->img_buffer = s->img_buffer_original;
    s->img_buffer_end = s->img_buffer_original_end;
}

// stb_image uses ints pervasively, including for offset calculations.
// therefore the largest decoded image size we can support with the
// current code, even on 64-bit targets, is INT_MAX. this is not a
// significant limitation for the intended use case.
//
// we do, however, need to make sure our size calculations don't
// overflow. hence a few helper functions for size calculations that
// multiply integers together, making sure that they're non-negative
// and no overflow occurs.

// return 1 if the sum is valid, 0 on overflow.
// negative terms are considered invalid.
static int giftk__addsizes_valid(int a, int b)
{
    if (b < 0)
        return 0;
    // now 0 <= b <= INT_MAX, hence also
    // 0 <= INT_MAX - b <= INTMAX.
    // And "a + b <= INT_MAX" (which might overflow) is the
    // same as a <= INT_MAX - b (no overflow)
    return a <= INT_MAX - b;
}

// returns 1 if the product is valid, 0 on overflow.
// negative factors are considered invalid.
static int giftk__mul2sizes_valid(int a, int b)
{
    if (a < 0 || b < 0)
        return 0;
    if (b == 0)
        return 1; // mul-by-0 is always safe
    // portable way to check for no overflows in a*b
    return a <= INT_MAX / b;
}

static int giftk__mad3sizes_valid(int a, int b, int c, int add)
{
    return giftk__mul2sizes_valid(a, b) && giftk__mul2sizes_valid(a * b, c) && giftk__addsizes_valid(a * b * c, add);
}

static void* giftk__malloc_mad3(int a, int b, int c, int add)
{
    if (!giftk__mad3sizes_valid(a, b, c, add))
        return NULL;
    return giftk__malloc(a * b * c + add);
}

#define giftk__err(x, y) giftk__err(x)
#define giftk__errpf(x, y) ((float*)(os_size)(giftk__err(x, y) ? NULL : NULL))
#define giftk__errpuc(x, y) ((unsigned char*)(os_size)(giftk__err(x, y) ? NULL : NULL))

// initialize a memory-decode context
static void giftk__start_mem(giftk__context* s, giftk_uc const* buffer, int len)
{
    s->io.read = NULL;
    s->read_from_callbacks = 0;
    s->callback_already_read = 0;
    s->img_buffer = s->img_buffer_original = (giftk_uc*)buffer;
    s->img_buffer_end = s->img_buffer_original_end = (giftk_uc*)buffer + len;
}
enum {
    GIFTK__SCAN_load = 0,
    GIFTK__SCAN_type,
    GIFTK__SCAN_header
};

static void giftk__refill_buffer(giftk__context* s)
{
    int n = (s->io.read)(s->io_user_data, (char*)s->buffer_start, s->buflen);
    s->callback_already_read += (int)(s->img_buffer - s->img_buffer_original);
    if (n == 0) {
        // at end of file, treat same as if from memory, but need to handle case
        // where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file
        s->read_from_callbacks = 0;
        s->img_buffer = s->buffer_start;
        s->img_buffer_end = s->buffer_start + 1;
        *s->img_buffer = 0;
    } else {
        s->img_buffer = s->buffer_start;
        s->img_buffer_end = s->buffer_start + n;
    }
}

giftk__inline static giftk_uc giftk__get8(giftk__context* s)
{
    if (s->img_buffer < s->img_buffer_end)
        return *s->img_buffer++;
    if (s->read_from_callbacks) {
        giftk__refill_buffer(s);
        return *s->img_buffer++;
    }
    return 0;
}

static void giftk__vertical_flip(void* image, int w, int h, int bytes_per_pixel)
{
    int row;
    os_size bytes_per_row = (os_size)w * bytes_per_pixel;
    giftk_uc temp[2048];
    giftk_uc* bytes = (giftk_uc*)image;

    for (row = 0; row < (h >> 1); row++) {
        giftk_uc* row0 = bytes + row * bytes_per_row;
        giftk_uc* row1 = bytes + (h - row - 1) * bytes_per_row;
        // swap row0 with row1
        os_size bytes_left = bytes_per_row;
        while (bytes_left) {
            os_size bytes_copy = (bytes_left < sizeof(temp)) ? bytes_left : sizeof(temp);
            memcpy(temp, row0, bytes_copy);
            memcpy(row0, row1, bytes_copy);
            memcpy(row1, temp, bytes_copy);
            row0 += bytes_copy;
            row1 += bytes_copy;
            bytes_left -= bytes_copy;
        }
    }
}

static void giftk__vertical_flip_slices(void* image, int w, int h, int z, int bytes_per_pixel)
{
    int slice;
    int slice_size = w * h * bytes_per_pixel;

    giftk_uc* bytes = (giftk_uc*)image;
    for (slice = 0; slice < z; ++slice) {
        giftk__vertical_flip(bytes, w, h, bytes_per_pixel);
        bytes += slice_size;
    }
}

GIFTK_DEF giftk_uc* giftk__load_gif_from_memory(giftk_uc const* buffer, int len, int** delays, int* x, int* y, int* z, int* comp, int req_comp)
{
    unsigned char* result;
    giftk__context s;
    giftk__start_mem(&s, buffer, len);

    result = (unsigned char*)giftk__load_gif_main(&s, delays, x, y, z, comp, req_comp);
    if (giftk__vertically_flip_on_load) {
        giftk__vertical_flip_slices(result, *x, *y, *z, *comp);
    }

    return result;
}

static void giftk__skip(giftk__context* s, int n)
{
    if (n == 0)
        return; // already there!
    if (n < 0) {
        s->img_buffer = s->img_buffer_end;
        return;
    }
    if (s->io.read) {
        int blen = (int)(s->img_buffer_end - s->img_buffer);
        if (blen < n) {
            s->img_buffer = s->img_buffer_end;
            (s->io.skip)(s->io_user_data, n - blen);
            return;
        }
    }
    s->img_buffer += n;
}

static int giftk__get16le(giftk__context* s)
{
    int z = giftk__get8(s);
    return z + (giftk__get8(s) << 8);
}

#define GIFTK__BYTECAST(x) ((giftk_uc)((x) & 255)) // truncate int to byte without warnings

//////////////////////////////////////////////////////////////////////////////
//
//  generic converter from built-in img_n to req_comp
//    individual types do this automatically as much as possible (e.g. jpeg
//    does all cases internally since it needs to colorspace convert anyway,
//    and it never has alpha, so very few cases ). png can automatically
//    interleave an alpha=255 channel, but falls back to this for other cases
//
//  assume data buffer is malloced, so malloc a new one and free that one
//  only failure mode is malloc failing

static giftk_uc giftk__compute_y(int r, int g, int b)
{
    return (giftk_uc)(((r * 77) + (g * 150) + (29 * b)) >> 8);
}

static unsigned char* giftk__convert_format(unsigned char* data, int img_n, int req_comp, unsigned int x, unsigned int y)
{
    int i, j;
    unsigned char* good;

    if (req_comp == img_n)
        return data;
    GIFTK_ASSERT(req_comp >= 1 && req_comp <= 4);

    good = (unsigned char*)giftk__malloc_mad3(req_comp, x, y, 0);
    if (good == NULL) {
        GIFTK_FREE(data);
        return giftk__errpuc("outofmem", "Out of memory");
    }

    for (j = 0; j < (int)y; ++j) {
        unsigned char* src = data + j * x * img_n;
        unsigned char* dest = good + j * x * req_comp;

#define GIFTK__COMBO(a, b) ((a) * 8 + (b))
#define GIFTK__CASE(a, b)    \
    case GIFTK__COMBO(a, b): \
        for (i = x - 1; i >= 0; --i, src += a, dest += b)
        // convert source image with img_n components to one with req_comp components;
        // avoid switch per pixel, so use switch per scanline and massive macros
        switch (GIFTK__COMBO(img_n, req_comp)) {
            GIFTK__CASE(1, 2)
            {
                dest[0] = src[0];
                dest[1] = 255;
            }
            break;
            GIFTK__CASE(1, 3) { dest[0] = dest[1] = dest[2] = src[0]; }
            break;
            GIFTK__CASE(1, 4)
            {
                dest[0] = dest[1] = dest[2] = src[0];
                dest[3] = 255;
            }
            break;
            GIFTK__CASE(2, 1) { dest[0] = src[0]; }
            break;
            GIFTK__CASE(2, 3) { dest[0] = dest[1] = dest[2] = src[0]; }
            break;
            GIFTK__CASE(2, 4)
            {
                dest[0] = dest[1] = dest[2] = src[0];
                dest[3] = src[1];
            }
            break;
            GIFTK__CASE(3, 4)
            {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = 255;
            }
            break;
            GIFTK__CASE(3, 1) { dest[0] = giftk__compute_y(src[0], src[1], src[2]); }
            break;
            GIFTK__CASE(3, 2)
            {
                dest[0] = giftk__compute_y(src[0], src[1], src[2]);
                dest[1] = 255;
            }
            break;
            GIFTK__CASE(4, 1) { dest[0] = giftk__compute_y(src[0], src[1], src[2]); }
            break;
            GIFTK__CASE(4, 2)
            {
                dest[0] = giftk__compute_y(src[0], src[1], src[2]);
                dest[1] = src[3];
            }
            break;
            GIFTK__CASE(4, 3)
            {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
            }
            break;
        default:
            GIFTK_ASSERT(0);
            GIFTK_FREE(data);
            GIFTK_FREE(good);
            return giftk__errpuc("unsupported", "Unsupported format conversion");
        }
#undef GIFTK__CASE
    }

    GIFTK_FREE(data);
    return good;
}

// *************************************************************************************************
// GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb

typedef struct
{
    giftk_int16 prefix;
    giftk_uc first;
    giftk_uc suffix;
} giftk__gif_lzw;

typedef struct
{
    int w, h;
    giftk_uc* out; // output buffer (always 4 components)
    giftk_uc* background; // The current "background" as far as a gif is concerned
    giftk_uc* history;
    int flags, bgindex, ratio, transparent, eflags;
    giftk_uc pal[256][4];
    giftk_uc lpal[256][4];
    giftk__gif_lzw codes[8192];
    giftk_uc* color_table;
    int parse, step;
    int lflags;
    int start_x, start_y;
    int max_x, max_y;
    int cur_x, cur_y;
    int line_size;
    int delay;
} giftk__gif;

static int giftk__gif_test_raw(giftk__context* s)
{
    int sz;
    if (giftk__get8(s) != 'G' || giftk__get8(s) != 'I' || giftk__get8(s) != 'F' || giftk__get8(s) != '8')
        return 0;
    sz = giftk__get8(s);
    if (sz != '9' && sz != '7')
        return 0;
    if (giftk__get8(s) != 'a')
        return 0;
    return 1;
}

static int giftk__gif_test(giftk__context* s)
{
    int r = giftk__gif_test_raw(s);
    giftk__rewind(s);
    return r;
}

static void giftk__gif_parse_colortable(giftk__context* s, giftk_uc pal[256][4], int num_entries, int transp)
{
    int i;
    for (i = 0; i < num_entries; ++i) {
        pal[i][2] = giftk__get8(s);
        pal[i][1] = giftk__get8(s);
        pal[i][0] = giftk__get8(s);
        pal[i][3] = transp == i ? 0 : 255;
    }
}

static int giftk__gif_header(giftk__context* s, giftk__gif* g, int* comp, int is_info)
{
    giftk_uc version;
    if (giftk__get8(s) != 'G' || giftk__get8(s) != 'I' || giftk__get8(s) != 'F' || giftk__get8(s) != '8')
        return giftk__err("not GIF", "Corrupt GIF");

    version = giftk__get8(s);
    if (version != '7' && version != '9')
        return giftk__err("not GIF", "Corrupt GIF");
    if (giftk__get8(s) != 'a')
        return giftk__err("not GIF", "Corrupt GIF");

    giftk__g_failure_reason = "";
    g->w = giftk__get16le(s);
    g->h = giftk__get16le(s);
    g->flags = giftk__get8(s);
    g->bgindex = giftk__get8(s);
    g->ratio = giftk__get8(s);
    g->transparent = -1;

    if (g->w > GIFTK_MAX_DIMENSIONS)
        return giftk__err("too large", "Very large image (corrupt?)");
    if (g->h > GIFTK_MAX_DIMENSIONS)
        return giftk__err("too large", "Very large image (corrupt?)");

    if (comp != 0)
        *comp = 4; // can't actually tell whether it's 3 or 4 until we parse the comments

    if (is_info)
        return 1;

    if (g->flags & 0x80)
        giftk__gif_parse_colortable(s, g->pal, 2 << (g->flags & 7), -1);

    return 1;
}

static int giftk__gif_info_raw(giftk__context* s, int* x, int* y, int* comp)
{
    giftk__gif* g = (giftk__gif*)giftk__malloc(sizeof(giftk__gif));
    if (!g)
        return giftk__err("outofmem", "Out of memory");
    if (!giftk__gif_header(s, g, comp, 1)) {
        GIFTK_FREE(g);
        giftk__rewind(s);
        return 0;
    }
    if (x)
        *x = g->w;
    if (y)
        *y = g->h;
    GIFTK_FREE(g);
    return 1;
}

static void giftk__out_gif_code(giftk__gif* g, giftk_uint16 code)
{
    giftk_uc *p, *c;
    int idx;

    // recurse to decode the prefixes, since the linked-list is backwards,
    // and working backwards through an interleaved image would be nasty
    if (g->codes[code].prefix >= 0)
        giftk__out_gif_code(g, g->codes[code].prefix);

    if (g->cur_y >= g->max_y)
        return;

    idx = g->cur_x + g->cur_y;
    p = &g->out[idx];
    g->history[idx / 4] = 1;

    c = &g->color_table[g->codes[code].suffix * 4];
    if (c[3] > 128) { // don't render transparent pixels;
        p[0] = c[2];
        p[1] = c[1];
        p[2] = c[0];
        p[3] = c[3];
    }
    g->cur_x += 4;

    if (g->cur_x >= g->max_x) {
        g->cur_x = g->start_x;
        g->cur_y += g->step;

        while (g->cur_y >= g->max_y && g->parse > 0) {
            g->step = (1 << g->parse) * g->line_size;
            g->cur_y = g->start_y + (g->step >> 1);
            --g->parse;
        }
    }
}

static giftk_uc* giftk__process_gif_raster(giftk__context* s, giftk__gif* g)
{
    giftk_uc lzw_cs;
    giftk_int32 len, init_code;
    giftk_uint32 first;
    giftk_int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
    giftk__gif_lzw* p;

    lzw_cs = giftk__get8(s);
    if (lzw_cs > 12)
        return NULL;
    clear = 1 << lzw_cs;
    first = 1;
    codesize = lzw_cs + 1;
    codemask = (1 << codesize) - 1;
    bits = 0;
    valid_bits = 0;
    for (init_code = 0; init_code < clear; init_code++) {
        g->codes[init_code].prefix = -1;
        g->codes[init_code].first = (giftk_uc)init_code;
        g->codes[init_code].suffix = (giftk_uc)init_code;
    }

    // support no starting clear code
    avail = clear + 2;
    oldcode = -1;

    len = 0;
    for (;;) {
        if (valid_bits < codesize) {
            if (len == 0) {
                len = giftk__get8(s); // start new block
                if (len == 0)
                    return g->out;
            }
            --len;
            bits |= (giftk_int32)giftk__get8(s) << valid_bits;
            valid_bits += 8;
        } else {
            giftk_int32 code = bits & codemask;
            bits >>= codesize;
            valid_bits -= codesize;
            // @OPTIMIZE: is there some way we can accelerate the non-clear path?
            if (code == clear) { // clear code
                codesize = lzw_cs + 1;
                codemask = (1 << codesize) - 1;
                avail = clear + 2;
                oldcode = -1;
                first = 0;
            } else if (code == clear + 1) { // end of stream code
                giftk__skip(s, len);
                while ((len = giftk__get8(s)) > 0)
                    giftk__skip(s, len);
                return g->out;
            } else if (code <= avail) {
                if (first) {
                    return giftk__errpuc("no clear code", "Corrupt GIF");
                }

                if (oldcode >= 0) {
                    p = &g->codes[avail++];
                    if (avail > 8192) {
                        return giftk__errpuc("too many codes", "Corrupt GIF");
                    }

                    p->prefix = (giftk_int16)oldcode;
                    p->first = g->codes[oldcode].first;
                    p->suffix = (code == avail) ? p->first : g->codes[code].first;
                } else if (code == avail)
                    return giftk__errpuc("illegal code in raster", "Corrupt GIF");

                giftk__out_gif_code(g, (giftk_uint16)code);

                if ((avail & codemask) == 0 && avail <= 0x0FFF) {
                    codesize++;
                    codemask = (1 << codesize) - 1;
                }

                oldcode = code;
            } else {
                return giftk__errpuc("illegal code in raster", "Corrupt GIF");
            }
        }
    }
}

// this function is designed to support animated gifs, although stb_image doesn't support it
// two back is the image from two frames ago, used for a very specific disposal format
static giftk_uc* giftk__gif_load_next(giftk__context* s, giftk__gif* g, int* comp, int req_comp, giftk_uc* two_back)
{
    int dispose;
    int first_frame;
    int pi;
    int pcount;
    GIFTK_NOTUSED(req_comp);

    // on first frame, any non-written pixels get the background colour (non-transparent)
    first_frame = 0;
    if (g->out == 0) {
        if (!giftk__gif_header(s, g, comp, 0))
            return 0; // giftk__g_failure_reason set by giftk__gif_header
        if (!giftk__mad3sizes_valid(4, g->w, g->h, 0))
            return giftk__errpuc("too large", "GIF image is too large");
        pcount = g->w * g->h;
        g->out = (giftk_uc*)giftk__malloc(4 * pcount);
        g->background = (giftk_uc*)giftk__malloc(4 * pcount);
        g->history = (giftk_uc*)giftk__malloc(pcount);
        if (!g->out || !g->background || !g->history)
            return giftk__errpuc("outofmem", "Out of memory");

        // image is treated as "transparent" at the start - ie, nothing overwrites the current background;
        // background colour is only used for pixels that are not rendered first frame, after that "background"
        // color refers to the color that was there the previous frame.
        memset(g->out, 0x00, 4 * pcount);
        memset(g->background, 0x00, 4 * pcount); // state of the background (starts transparent)
        memset(g->history, 0x00, pcount); // pixels that were affected previous frame
        first_frame = 1;
    } else {
        // second frame - how do we dispose of the previous one?
        dispose = (g->eflags & 0x1C) >> 2;
        pcount = g->w * g->h;

        if ((dispose == 3) && (two_back == 0)) {
            dispose = 2; // if I don't have an image to revert back to, default to the old background
        }

        if (dispose == 3) { // use previous graphic
            for (pi = 0; pi < pcount; ++pi) {
                if (g->history[pi]) {
                    memcpy(&g->out[pi * 4], &two_back[pi * 4], 4);
                }
            }
        } else if (dispose == 2) {
            // restore what was changed last frame to background before that frame;
            for (pi = 0; pi < pcount; ++pi) {
                if (g->history[pi]) {
                    memcpy(&g->out[pi * 4], &g->background[pi * 4], 4);
                }
            }
        } else {
            // This is a non-disposal case eithe way, so just
            // leave the pixels as is, and they will become the new background
            // 1: do not dispose
            // 0:  not specified.
        }

        // background is what out is after the undoing of the previou frame;
        memcpy(g->background, g->out, 4 * g->w * g->h);
    }

    // clear my history;
    memset(g->history, 0x00, g->w * g->h); // pixels that were affected previous frame

    for (;;) {
        int tag = giftk__get8(s);
        switch (tag) {
        case 0x2C: /* Image Descriptor */
        {
            giftk_int32 x, y, w, h;
            giftk_uc* o;

            x = giftk__get16le(s);
            y = giftk__get16le(s);
            w = giftk__get16le(s);
            h = giftk__get16le(s);
            if (((x + w) > (g->w)) || ((y + h) > (g->h)))
                return giftk__errpuc("bad Image Descriptor", "Corrupt GIF");

            g->line_size = g->w * 4;
            g->start_x = x * 4;
            g->start_y = y * g->line_size;
            g->max_x = g->start_x + w * 4;
            g->max_y = g->start_y + h * g->line_size;
            g->cur_x = g->start_x;
            g->cur_y = g->start_y;

            // if the width of the specified rectangle is 0, that means
            // we may not see *any* pixels or the image is malformed;
            // to make sure this is caught, move the current y down to
            // max_y (which is what out_gif_code checks).
            if (w == 0)
                g->cur_y = g->max_y;

            g->lflags = giftk__get8(s);

            if (g->lflags & 0x40) {
                g->step = 8 * g->line_size; // first interlaced spacing
                g->parse = 3;
            } else {
                g->step = g->line_size;
                g->parse = 0;
            }

            if (g->lflags & 0x80) {
                giftk__gif_parse_colortable(s, g->lpal, 2 << (g->lflags & 7), g->eflags & 0x01 ? g->transparent : -1);
                g->color_table = (giftk_uc*)g->lpal;
            } else if (g->flags & 0x80) {
                g->color_table = (giftk_uc*)g->pal;
            } else
                return giftk__errpuc("missing color table", "Corrupt GIF");

            o = giftk__process_gif_raster(s, g);
            if (!o)
                return NULL;

            // if this was the first frame,
            pcount = g->w * g->h;
            if (first_frame && (g->bgindex > 0)) {
                // if first frame, any pixel not drawn to gets the background color
                for (pi = 0; pi < pcount; ++pi) {
                    if (g->history[pi] == 0) {
                        g->pal[g->bgindex][3] = 255; // just in case it was made transparent, undo that; It will be reset next frame if need be;
                        memcpy(&g->out[pi * 4], &g->pal[g->bgindex], 4);
                    }
                }
            }

            return o;
        }

        case 0x21: // Comment Extension.
        {
            int len;
            int ext = giftk__get8(s);
            if (ext == 0xF9) { // Graphic Control Extension.
                len = giftk__get8(s);
                if (len == 4) {
                    g->eflags = giftk__get8(s);
                    g->delay = 10 * giftk__get16le(s); // delay - 1/100th of a second, saving as 1/1000ths.

                    // unset old transparent
                    if (g->transparent >= 0) {
                        g->pal[g->transparent][3] = 255;
                    }
                    if (g->eflags & 0x01) {
                        g->transparent = giftk__get8(s);
                        if (g->transparent >= 0) {
                            g->pal[g->transparent][3] = 0;
                        }
                    } else {
                        // don't need transparent
                        giftk__skip(s, 1);
                        g->transparent = -1;
                    }
                } else {
                    giftk__skip(s, len);
                    break;
                }
            }
            while ((len = giftk__get8(s)) != 0) {
                giftk__skip(s, len);
            }
            break;
        }

        case 0x3B: // gif stream termination code
            return (giftk_uc*)s; // using '1' causes warning on some compilers

        default:
            return giftk__errpuc("unknown code", "Corrupt GIF");
        }
    }
}

static void* giftk__load_gif_main_outofmem(giftk__gif* g, giftk_uc* out, int** delays)
{
    GIFTK_FREE(g->out);
    GIFTK_FREE(g->history);
    GIFTK_FREE(g->background);

    if (out)
        GIFTK_FREE(out);
    if (delays && *delays)
        GIFTK_FREE(*delays);
    return giftk__errpuc("outofmem", "Out of memory");
}

static void* giftk__load_gif_main(giftk__context* s, int** delays, int* x, int* y, int* z, int* comp, int req_comp)
{
    if (giftk__gif_test(s)) {
        int layers = 0;
        giftk_uc* u = 0;
        giftk_uc* out = 0;
        giftk_uc* two_back = 0;
        giftk__gif g;
        int stride;
        int out_size = 0;
        int delays_size = 0;

        GIFTK_NOTUSED(out_size);
        GIFTK_NOTUSED(delays_size);

        memset(&g, 0, sizeof(g));
        if (delays) {
            *delays = 0;
        }

        do {
            u = giftk__gif_load_next(s, &g, comp, req_comp, two_back);
            if (u == (giftk_uc*)s)
                u = 0; // end of animated gif marker

            if (u) {
                *x = g.w;
                *y = g.h;
                ++layers;
                stride = g.w * g.h * 4;

                if (out) {
                    void* tmp = (giftk_uc*)GIFTK_REALLOC_SIZED(out, out_size, layers * stride);
                    if (!tmp)
                        return giftk__load_gif_main_outofmem(&g, out, delays);
                    else {
                        out = (giftk_uc*)tmp;
                        out_size = layers * stride;
                    }

                    if (delays) {
                        int* new_delays = (int*)GIFTK_REALLOC_SIZED(*delays, delays_size, sizeof(int) * layers);
                        if (!new_delays)
                            return giftk__load_gif_main_outofmem(&g, out, delays);
                        *delays = new_delays;
                        delays_size = layers * sizeof(int);
                    }
                } else {
                    out = (giftk_uc*)giftk__malloc(layers * stride);
                    if (!out)
                        return giftk__load_gif_main_outofmem(&g, out, delays);
                    out_size = layers * stride;
                    if (delays) {
                        *delays = (int*)giftk__malloc(layers * sizeof(int));
                        if (!*delays)
                            return giftk__load_gif_main_outofmem(&g, out, delays);
                        delays_size = layers * sizeof(int);
                    }
                }
                memcpy(out + ((layers - 1) * stride), u, stride);
                if (layers >= 2) {
                    two_back = out - 2 * stride;
                }

                if (delays) {
                    (*delays)[layers - 1U] = g.delay;
                }
            }
        } while (u != 0);

        // free temp buffer;
        GIFTK_FREE(g.out);
        GIFTK_FREE(g.history);
        GIFTK_FREE(g.background);

        // do the final conversion after loading everything;
        if (req_comp && req_comp != 4)
            out = giftk__convert_format(out, 4, req_comp, layers * g.w, g.h);

        *z = layers;
        return out;
    } else {
        return giftk__errpuc("not GIF", "Image was not as a gif type.");
    }
}

static void* giftk__gif_load(giftk__context* s, int* x, int* y, int* comp, int req_comp, giftk__result_info* ri)
{
    giftk_uc* u = 0;
    giftk__gif g;
    memset(&g, 0, sizeof(g));
    GIFTK_NOTUSED(ri);

    u = giftk__gif_load_next(s, &g, comp, req_comp, 0);
    if (u == (giftk_uc*)s)
        u = 0; // end of animated gif marker
    if (u) {
        *x = g.w;
        *y = g.h;

        // moved conversion to after successful load so that the same
        // can be done for multiple frames.
        if (req_comp && req_comp != 4)
            u = giftk__convert_format(u, 4, req_comp, g.w, g.h);
    } else if (g.out) {
        // if there was an error and we allocated an image buffer, free it!
        GIFTK_FREE(g.out);
    }

    // free buffers needed for multiple frame loading;
    GIFTK_FREE(g.history);
    GIFTK_FREE(g.background);

    return u;
}

static int giftk__gif_info(giftk__context* s, int* x, int* y, int* comp)
{
    return giftk__gif_info_raw(s, x, y, comp);
}

GIFTK_DEF struct giftk__context* giftk__init(char const* file)
{
    int i;
    struct giftk__context* gif;
    struct giftk__result_info ri;
    int comp = 3;
    int req_comp = 3;
    unsigned char* buffer;
    int len;
    FILE* fp;

    gif = malloc(sizeof(struct giftk__context));
    if (!gif) {
        return NULL;
    }

    memset(gif, 0, sizeof(struct giftk__context));
    for (i = 0; i < sizeof(gif->file) - 1; i++) {
        gif->file[i] = file[i];
    }

    fp = fopen(file, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: cannot open %s\n", file);
        exit(-1);
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if (len < 1) {
        fprintf(stderr, "ERROR: %s is empty\n", file);
        exit(-1);
    }
    fseek(fp, 0, SEEK_SET);
    buffer = malloc(len);
    fread(buffer, 1, len, fp);
    fclose(fp);

    giftk__start_mem(gif, buffer, len);

    memset(&ri, 0, sizeof(ri));
    gif->output = giftk__load_gif_main(gif, &gif->delays, &gif->img_x, &gif->img_y, &gif->img_z, &comp, req_comp);
    gif->origin = gif->output;
    free(buffer);
    return gif;
}

GIFTK_DEF int giftk__next(struct giftk__context* gif)
{
    int comp = 3;
    int req_comp = 3;
    int delay = 10;

    if (gif->img_z < 2) {
        return -1;
    }
    gif->current_z++;
    if (gif->current_z >= gif->img_z) {
        gif->current_z = 0;
    }
    gif->output = gif->origin + (gif->img_x * gif->img_y * 3 * gif->current_z);
    return gif->delays[gif->current_z];
}

GIFTK_DEF void giftk__shutdown(struct giftk__context* gif)
{
    if (gif->origin) {
        free(gif->origin);
    }
    free(gif);
}



//////////////////////////////////////////////////////////////////////////////
//
//  Rasterizer

typedef struct giftk__hheap_chunk
{
    struct giftk__hheap_chunk* next;
} giftk__hheap_chunk;

typedef struct giftk__hheap
{
    struct giftk__hheap_chunk* head;
    void* first_free;
    int    num_remaining_in_head_chunk;
} giftk__hheap;

static void* giftk__hheap_alloc(giftk__hheap* hh, os_size size, void* userdata)
{
    if (hh->first_free) {
        void* p = hh->first_free;
        hh->first_free = *(void**)p;
        return p;
    }
    else {
        if (hh->num_remaining_in_head_chunk == 0) {
            int count = (size < 32 ? 2000 : size < 128 ? 800 : 100);
            giftk__hheap_chunk* c = (giftk__hheap_chunk*)GIFTK_malloc(sizeof(giftk__hheap_chunk) + size * count, userdata);
            if (c == NULL)
                return NULL;
            c->next = hh->head;
            hh->head = c;
            hh->num_remaining_in_head_chunk = count;
        }
        --hh->num_remaining_in_head_chunk;
        return (char*)(hh->head) + sizeof(giftk__hheap_chunk) + size * hh->num_remaining_in_head_chunk;
    }
}

static void giftk__hheap_free(giftk__hheap* hh, void* p)
{
    *(void**)p = hh->first_free;
    hh->first_free = p;
}

static void giftk__hheap_cleanup(giftk__hheap* hh, void* userdata)
{
    giftk__hheap_chunk* c = hh->head;
    while (c) {
        giftk__hheap_chunk* n = c->next;
        GIFTK_free(c, userdata);
        c = n;
    }
}

typedef struct giftk__edge {
    float x0, y0, x1, y1;
    int invert;
} giftk__edge;


typedef struct giftk__active_edge
{
    struct giftk__active_edge* next;
    int x, dx;
    float ey;
    int direction;
} giftk__active_edge;


#define GIFTK_FIXSHIFT   10
#define GIFTK_FIX        (1 << GIFTK_FIXSHIFT)
#define GIFTK_FIXMASK    (GIFTK_FIX-1)

static giftk__active_edge* giftk__new_active(giftk__hheap* hh, giftk__edge* e, int off_x, float start_point, void* userdata)
{
    giftk__active_edge* z = (giftk__active_edge*)giftk__hheap_alloc(hh, sizeof(*z), userdata);
    float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
    GIFTK_assert(z != NULL);
    if (!z) return z;

    // round dx down to avoid overshooting
    if (dxdy < 0)
        z->dx = -GIFTK_ifloor(GIFTK_FIX * -dxdy);
    else
        z->dx = GIFTK_ifloor(GIFTK_FIX * dxdy);

    z->x = GIFTK_ifloor(GIFTK_FIX * e->x0 + z->dx * (start_point - e->y0)); // use z->dx so when we offset later it's by the same amount
    z->x -= off_x * GIFTK_FIX;

    z->ey = e->y1;
    z->next = 0;
    z->direction = e->invert ? 1 : -1;
    return z;
}



// note: this routine clips fills that extend off the edges... ideally this
// wouldn't happen, but it could happen if the truetype glyph bounding boxes
// are wrong, or if the user supplies a too-small bitmap
static void giftk__fill_active_edges(unsigned char* scanline, int len, giftk__active_edge* e, int max_weight)
{
    // non-zero winding fill
    int x0 = 0, w = 0;

    while (e) {
        if (w == 0) {
            // if we're currently at zero, we need to record the edge start point
            x0 = e->x; w += e->direction;
        }
        else {
            int x1 = e->x; w += e->direction;
            // if we went to zero, we need to draw
            if (w == 0) {
                int i = x0 >> GIFTK_FIXSHIFT;
                int j = x1 >> GIFTK_FIXSHIFT;

                if (i < len && j >= 0) {
                    if (i == j) {
                        // x0,x1 are the same pixel, so compute combined coverage
                        scanline[i] = scanline[i] + (giftk_uint8)((x1 - x0) * max_weight >> GIFTK_FIXSHIFT);
                    }
                    else {
                        if (i >= 0) // add antialiasing for x0
                            scanline[i] = scanline[i] + (giftk_uint8)(((GIFTK_FIX - (x0 & GIFTK_FIXMASK)) * max_weight) >> GIFTK_FIXSHIFT);
                        else
                            i = -1; // clip

                        if (j < len) // add antialiasing for x1
                            scanline[j] = scanline[j] + (giftk_uint8)(((x1 & GIFTK_FIXMASK) * max_weight) >> GIFTK_FIXSHIFT);
                        else
                            j = len; // clip

                        for (++i; i < j; ++i) // fill pixels between x0 and x1
                            scanline[i] = scanline[i] + (giftk_uint8)max_weight;
                    }
                }
            }
        }

        e = e->next;
    }
}

static void giftk__blend_over(giftk__bitmap* result, int y, unsigned char* scanline, struct nk_color c)
{
    unsigned char* p = result->pixels + 3 * y * result->stride;
    int w = result->w;
    int i;
    for (i = 0; i < w; i++) {
        int a = (scanline[i] * c.a) / 255;
        if (a >= 255) {
            *p = c.b; p++;
            *p = c.g; p++;
            *p = c.r; p++;
        }
        else if (a > 0) {
            int ab = (255 - a);
            int v;
            v = ((int)(*p) * ab + c.b * a) / 255;
            *p = (v <= 255) ? v : 255; p++;
            v = ((int)(*p) * ab + c.g * a) / 255;
            *p = (v <= 255) ? v : 255; p++;
            v = ((int)(*p) * ab + c.r * a) / 255;
            *p = (v <= 255) ? v : 255; p++;
        }
        else {
            p += 3;
        }
    }
}

static void giftk__rasterize_sorted_edges(giftk__bitmap* result, giftk__edge* e, int n, int vsubsample, int off_x, int off_y, void* userdata, struct nk_color c)
{
    giftk__hheap hh = { 0, 0, 0 };
    giftk__active_edge* active = NULL;
    int y, j = 0;
    int max_weight = (255 / vsubsample);  // weight per vertical scanline
    int s; // vertical subsample index
    unsigned char scanline_data[512], * scanline;

    if (result->w > 512)
        scanline = (unsigned char*)GIFTK_malloc(result->w, userdata);
    else
        scanline = scanline_data;

    y = off_y * vsubsample;
    e[n].y0 = (off_y + result->h) * (float)vsubsample + 1;

    while (j < result->h) {
        GIFTK_memset(scanline, 0, result->w);
        for (s = 0; s < vsubsample; ++s) {
            // find center of pixel for this scanline
            float scan_y = y + 0.5f;
            giftk__active_edge** step = &active;

            // update all active edges;
            // remove all active edges that terminate before the center of this scanline
            while (*step) {
                giftk__active_edge* z = *step;
                if (z->ey <= scan_y) {
                    *step = z->next; // delete from list
                    GIFTK_assert(z->direction);
                    z->direction = 0;
                    giftk__hheap_free(&hh, z);
                }
                else {
                    z->x += z->dx; // advance to position for current scanline
                    step = &((*step)->next); // advance through list
                }
            }

            // resort the list if needed
            for (;;) {
                int changed = 0;
                step = &active;
                while (*step && (*step)->next) {
                    if ((*step)->x > (*step)->next->x) {
                        giftk__active_edge* t = *step;
                        giftk__active_edge* q = t->next;

                        t->next = q->next;
                        q->next = t;
                        *step = q;
                        changed = 1;
                    }
                    step = &(*step)->next;
                }
                if (!changed) break;
            }

            // insert all edges that start before the center of this scanline -- omit ones that also end on this scanline
            while (e->y0 <= scan_y) {
                if (e->y1 > scan_y) {
                    giftk__active_edge* z = giftk__new_active(&hh, e, off_x, scan_y, userdata);
                    if (z != NULL) {
                        // find insertion point
                        if (active == NULL)
                            active = z;
                        else if (z->x < active->x) {
                            // insert at front
                            z->next = active;
                            active = z;
                        }
                        else {
                            // find thing to insert AFTER
                            giftk__active_edge* p = active;
                            while (p->next && p->next->x < z->x)
                                p = p->next;
                            // at this point, p->next->x is NOT < z->x
                            z->next = p->next;
                            p->next = z;
                        }
                    }
                }
                ++e;
            }

            // now process all active edges in XOR fashion
            if (active)
                giftk__fill_active_edges(scanline, result->w, active, max_weight);

            ++y;
        }
        giftk__blend_over(result, j, scanline, c);
        //GIFTK_memcpy(result->pixels + j * result->stride, scanline, result->w);
        ++j;
    }

    giftk__hheap_cleanup(&hh, userdata);

    if (scanline != scanline_data)
        GIFTK_free(scanline, userdata);
}


#define GIFTK__COMPARE(a,b)  ((a)->y0 < (b)->y0)

static void giftk__sort_edges_ins_sort(giftk__edge* p, int n)
{
    int i, j;
    for (i = 1; i < n; ++i) {
        giftk__edge t = p[i], * a = &t;
        j = i;
        while (j > 0) {
            giftk__edge* b = &p[j - 1];
            int c = GIFTK__COMPARE(a, b);
            if (!c) break;
            p[j] = p[j - 1];
            --j;
        }
        if (i != j)
            p[j] = t;
    }
}

static void giftk__sort_edges_quicksort(giftk__edge* p, int n)
{
    /* threshold for transitioning to insertion sort */
    while (n > 12) {
        giftk__edge t;
        int c01, c12, c, m, i, j;

        /* compute median of three */
        m = n >> 1;
        c01 = GIFTK__COMPARE(&p[0], &p[m]);
        c12 = GIFTK__COMPARE(&p[m], &p[n - 1]);
        /* if 0 >= mid >= end, or 0 < mid < end, then use mid */
        if (c01 != c12) {
            /* otherwise, we'll need to swap something else to middle */
            int z;
            c = GIFTK__COMPARE(&p[0], &p[n - 1]);
            /* 0>mid && mid<n:  0>n => n; 0<n => 0 */
            /* 0<mid && mid>n:  0>n => 0; 0<n => n */
            z = (c == c12) ? 0 : n - 1;
            t = p[z];
            p[z] = p[m];
            p[m] = t;
        }
        /* now p[m] is the median-of-three */
        /* swap it to the beginning so it won't move around */
        t = p[0];
        p[0] = p[m];
        p[m] = t;

        /* partition loop */
        i = 1;
        j = n - 1;
        for (;;) {
            /* handling of equality is crucial here */
            /* for sentinels & efficiency with duplicates */
            for (;; ++i) {
                if (!GIFTK__COMPARE(&p[i], &p[0])) break;
            }
            for (;; --j) {
                if (!GIFTK__COMPARE(&p[0], &p[j])) break;
            }
            /* make sure we haven't crossed */
            if (i >= j) break;
            t = p[i];
            p[i] = p[j];
            p[j] = t;

            ++i;
            --j;
        }
        /* recurse on smaller side, iterate on larger */
        if (j < (n - i)) {
            giftk__sort_edges_quicksort(p, j);
            p = p + i;
            n = n - i;
        }
        else {
            giftk__sort_edges_quicksort(p + i, n - i);
            n = j;
        }
    }
}

static void giftk__sort_edges(giftk__edge* p, int n)
{
    giftk__sort_edges_quicksort(p, n);
    giftk__sort_edges_ins_sort(p, n);
}

typedef struct
{
    float x, y;
} giftk__point;

static void giftk__rasterize(giftk__bitmap* result, giftk__point* pts, int* wcount, int windings, float scale_x, float scale_y, float shift_x, float shift_y, int off_x, int off_y, int invert, void* userdata, struct nk_color c)
{
    float y_scale_inv = invert ? -scale_y : scale_y;
    giftk__edge* e;
    int n, i, j, k, m;

    int vsubsample = result->h < 8 ? 15 : 5;

    // vsubsample should divide 255 evenly; otherwise we won't reach full opacity

    // now we have to blow out the windings into explicit edge lists
    n = 0;
    for (i = 0; i < windings; ++i)
        n += wcount[i];

    e = (giftk__edge*)GIFTK_malloc(sizeof(*e) * (n + 1), userdata); // add an extra one as a sentinel
    if (e == 0) return;
    n = 0;

    m = 0;
    for (i = 0; i < windings; ++i) {
        giftk__point* p = pts + m;
        m += wcount[i];
        j = wcount[i] - 1;
        for (k = 0; k < wcount[i]; j = k++) {
            int a = k, b = j;
            // skip the edge if horizontal
            if (p[j].y == p[k].y)
                continue;
            // add edge from j to k to the list
            e[n].invert = 0;
            if (invert ? p[j].y > p[k].y : p[j].y < p[k].y) {
                e[n].invert = 1;
                a = j, b = k;
            }
            e[n].x0 = p[a].x * scale_x + shift_x;
            e[n].y0 = (p[a].y * y_scale_inv + shift_y) * vsubsample;
            e[n].x1 = p[b].x * scale_x + shift_x;
            e[n].y1 = (p[b].y * y_scale_inv + shift_y) * vsubsample;
            ++n;
        }
    }

    // now sort the edges by their highest point (should snap to integer, and then by x)
    //GIFTK_sort(e, n, sizeof(e[0]), giftk__edge_compare);
    giftk__sort_edges(e, n);

    // now, traverse the scanlines and find the intersections on each scanline, use xor winding rule
    giftk__rasterize_sorted_edges(result, e, n, vsubsample, off_x, off_y, userdata, c);

    GIFTK_free(e, userdata);
}

static void giftk__add_point(giftk__point* points, int n, float x, float y)
{
    if (!points) return; // during first pass, it's unallocated
    points[n].x = x;
    points[n].y = y;
}

// tessellate until threshold p is happy... @TODO warped to compensate for non-linear stretching
static int giftk__tesselate_curve(giftk__point* points, int* num_points, float x0, float y0, float x1, float y1, float x2, float y2, float objspace_flatness_squared, int n)
{
    // midpoint
    float mx = (x0 + 2 * x1 + x2) / 4;
    float my = (y0 + 2 * y1 + y2) / 4;
    // versus directly drawn line
    float dx = (x0 + x2) / 2 - mx;
    float dy = (y0 + y2) / 2 - my;
    if (n > 16) // 65536 segments on one curve better be enough!
        return 1;
    if (dx * dx + dy * dy > objspace_flatness_squared) { // half-pixel error allowed... need to be smaller if AA
        giftk__tesselate_curve(points, num_points, x0, y0, (x0 + x1) / 2.0f, (y0 + y1) / 2.0f, mx, my, objspace_flatness_squared, n + 1);
        giftk__tesselate_curve(points, num_points, mx, my, (x1 + x2) / 2.0f, (y1 + y2) / 2.0f, x2, y2, objspace_flatness_squared, n + 1);
    }
    else {
        giftk__add_point(points, *num_points, x2, y2);
        *num_points = *num_points + 1;
    }
    return 1;
}

static void giftk__tesselate_cubic(giftk__point* points, int* num_points, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float objspace_flatness_squared, int n)
{
    // @TODO this "flatness" calculation is just made-up nonsense that seems to work well enough
    float dx0 = x1 - x0;
    float dy0 = y1 - y0;
    float dx1 = x2 - x1;
    float dy1 = y2 - y1;
    float dx2 = x3 - x2;
    float dy2 = y3 - y2;
    float dx = x3 - x0;
    float dy = y3 - y0;
    float longlen = (float)(GIFTK_sqrt(dx0 * dx0 + dy0 * dy0) + GIFTK_sqrt(dx1 * dx1 + dy1 * dy1) + GIFTK_sqrt(dx2 * dx2 + dy2 * dy2));
    float shortlen = (float)GIFTK_sqrt(dx * dx + dy * dy);
    float flatness_squared = longlen * longlen - shortlen * shortlen;

    if (n > 16) // 65536 segments on one curve better be enough!
        return;

    if (flatness_squared > objspace_flatness_squared) {
        float x01 = (x0 + x1) / 2;
        float y01 = (y0 + y1) / 2;
        float x12 = (x1 + x2) / 2;
        float y12 = (y1 + y2) / 2;
        float x23 = (x2 + x3) / 2;
        float y23 = (y2 + y3) / 2;

        float xa = (x01 + x12) / 2;
        float ya = (y01 + y12) / 2;
        float xb = (x12 + x23) / 2;
        float yb = (y12 + y23) / 2;

        float mx = (xa + xb) / 2;
        float my = (ya + yb) / 2;

        giftk__tesselate_cubic(points, num_points, x0, y0, x01, y01, xa, ya, mx, my, objspace_flatness_squared, n + 1);
        giftk__tesselate_cubic(points, num_points, mx, my, xb, yb, x23, y23, x3, y3, objspace_flatness_squared, n + 1);
    }
    else {
        giftk__add_point(points, *num_points, x3, y3);
        *num_points = *num_points + 1;
    }
}



// returns number of contours
static giftk__point* giftk_FlattenCurves(giftk__vertex* vertices, int num_verts, float objspace_flatness, int** contour_lengths, int* num_contours, void* userdata)
{
    giftk__point* points = 0;
    int num_points = 0;

    float objspace_flatness_squared = objspace_flatness * objspace_flatness;
    int i, n = 0, start = 0, pass;

    // count how many "moves" there are to get the contour count
    for (i = 0; i < num_verts; ++i)
        if (vertices[i].type == GIFTK_vmove)
            ++n;

    *num_contours = n;
    if (n == 0) return 0;

    *contour_lengths = (int*)GIFTK_malloc(sizeof(**contour_lengths) * n, userdata);

    if (*contour_lengths == 0) {
        *num_contours = 0;
        return 0;
    }

    // make two passes through the points so we don't need to realloc
    for (pass = 0; pass < 2; ++pass) {
        float x = 0, y = 0;
        if (pass == 1) {
            points = (giftk__point*)GIFTK_malloc(num_points * sizeof(points[0]), userdata);
            if (points == NULL) goto error;
        }
        num_points = 0;
        n = -1;
        for (i = 0; i < num_verts; ++i) {
            switch (vertices[i].type) {
            case GIFTK_vmove:
                // start the next contour
                if (n >= 0)
                    (*contour_lengths)[n] = num_points - start;
                ++n;
                start = num_points;

                x = vertices[i].x, y = vertices[i].y;
                giftk__add_point(points, num_points++, x, y);
                break;
            case GIFTK_vline:
                x = vertices[i].x, y = vertices[i].y;
                giftk__add_point(points, num_points++, x, y);
                break;
            case GIFTK_vcurve:
                giftk__tesselate_curve(points, &num_points, x, y,
                    vertices[i].cx, vertices[i].cy,
                    vertices[i].x, vertices[i].y,
                    objspace_flatness_squared, 0);
                x = vertices[i].x, y = vertices[i].y;
                break;
            case GIFTK_vcubic:
                giftk__tesselate_cubic(points, &num_points, x, y,
                    vertices[i].cx, vertices[i].cy,
                    vertices[i].cx1, vertices[i].cy1,
                    vertices[i].x, vertices[i].y,
                    objspace_flatness_squared, 0);
                x = vertices[i].x, y = vertices[i].y;
                break;
            case GIFTK_varc:
                giftk__tesselate_arc(points, &num_points, x, y,
                    vertices[i].cx, vertices[i].cy,
                    vertices[i].x, vertices[i].y,
                    objspace_flatness_squared, 0);
                x = vertices[i].x, y = vertices[i].y;
                break;
            }
        }
        (*contour_lengths)[n] = num_points - start;
    }

    return points;
error:
    GIFTK_free(points, userdata);
    GIFTK_free(*contour_lengths, userdata);
    *contour_lengths = 0;
    *num_contours = 0;
    return NULL;
}

GIFTK_DEF void giftk__render(struct giftk__context* gif, float flatness_in_pixels, giftk__vertex* vertices, int num_verts, float scale_x, float scale_y, float shift_x, float shift_y, int x_off, int y_off, int invert, void* userdata, struct nk_color c)
{
    giftk__bitmap gbm;
    float scale = scale_x > scale_y ? scale_y : scale_x;
    int winding_count = 0;
    int* winding_lengths = NULL;
    giftk__point* windings = giftk_FlattenCurves(vertices, num_verts, flatness_in_pixels / scale, &winding_lengths, &winding_count, userdata);

    gbm.w = gif->img_x;
    gbm.h = gif->img_y;
    gbm.pixels = gif->output;
    gbm.stride = gbm.w;

    if (windings) {
        giftk__rasterize(&gbm, windings, winding_lengths, winding_count, scale_x, scale_y, shift_x, shift_y, x_off, y_off, invert, userdata, c);
        GIFTK_free(winding_lengths, userdata);
        GIFTK_free(windings, userdata);
    }
}

NK_API void giftk__setvertex(giftk__vertex* v, giftk_uint8 type, giftk_int32 x, giftk_int32 y, giftk_int32 cx, giftk_int32 cy)
{
    v->type = type;
    v->x = (giftk_int16)x;
    v->y = (giftk_int16)y;
    v->cx = (giftk_int16)cx;
    v->cy = (giftk_int16)cy;
}


NK_API void
giftk__fill_rect(struct giftk__context* gif, struct nk_rect r,
    float rounding, struct nk_color c)
{
    int num_vertices = 0;
    giftk__vertex vertices[4];
    int x, y, w, h;
    x = (int)r.x;
    y = (int)r.y;
    w = (int)r.w;
    h = (int)r.h;

    giftk__setvertex(&vertices[num_vertices++], GIFTK_vmove, x, y, 0, 0);
    giftk__setvertex(&vertices[num_vertices++], GIFTK_vline, x + w, y, 0, 0);
    giftk__setvertex(&vertices[num_vertices++], GIFTK_vline, x + w, y + h, 0, 0);
    giftk__setvertex(&vertices[num_vertices++], GIFTK_vline, x, y + h, 0, 0);
    giftk__render(gif, 0.35f, vertices, num_vertices, 1.0, 1.0, 0, 0, 0, 0, 0, NULL, c);

}

static void
NK_API void
giftk__fill_circle(struct giftk__context* gif, struct nk_rect r,
    float rounding, struct nk_color c)
{
    int num_vertices = 0;
    float c = 0.55228474983079;
    giftk__vertex vertices[4];
    int x, y, w, h;
    x = (int)r.x;
    y = (int)r.y;
    w = (int)r.w;
    h = (int)r.h;

    giftk__setvertex(&vertices[num_vertices++], GIFTK_vmove, x, y, 0, 0);
    giftk__setvertex(&vertices[num_vertices++], GIFTK_vline, x + w, y, 0, 0);
    giftk__setvertex(&vertices[num_vertices++], GIFTK_vline, x + w, y + h, 0, 0);
    giftk__setvertex(&vertices[num_vertices++], GIFTK_vline, x, y + h, 0, 0);
    giftk__render(gif, 0.35f, vertices, num_vertices, 1.0, 1.0, 0, 0, 0, 0, 0, NULL, c);

}




#endif /* GIFTK_IMPLEMENTATION */

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
