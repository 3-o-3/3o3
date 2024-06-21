/*
 *                         OS-3o3 Operating System
 *
 *            some C functions equivalent for use in the kernel
 *
 *                      13 may MMXXIV PUBLIC DOMAIN
 *           The authors disclaim copyright to this source code.
 *
 *
 */

#include "../include/klib.h"
#include "../include/rtos.h"

#include "font.h"
#define FONT_HEIGHT 16
#define FONT_WIDTH 8

os_uint32 k__shell_fg = 0xFFFFFFFF;
os_uint32 k__shell_bg = 0xFF000000;
os_int32 k__shell_x = 0;
os_int32 k__shell_y = 0;

static void *k__heap_ptr = NULL;
static os_size k__heap_size = K__ALIGNED_HEAP_SIZE;

/**
 * allocate aligned memory for use with XHCI buffers
 */
void *k__alloc_aligned(os_size size, os_size align, os_size boundary)
{
    os_size ptr, next_bound;
    if (k__heap_ptr == NULL)
    {
        k__heap_ptr = k__heap_alloc(HEAP_SYSTEM, k__heap_size);
    }

    ptr = (os_size)k__heap_ptr;
    ptr = (ptr + align - 1) & ~(align - 1);
    if (boundary > 0)
    {
        next_bound = (ptr + boundary - 1) & ~(boundary - 1);
        if (ptr + size > next_bound)
        {
            ptr = next_bound;
        }
    }
    size = size + ptr - (os_size)k__heap_ptr;
    if (size > k__heap_size)
    {
        k__printf("\nCannot alloc aligned memory.\n");
        return NULL;
    }
    k__heap_size -= size;
    k__heap_ptr = (void *)(ptr + size);
    return (void *)ptr;
}

os_intn k__memcmp(void *str1, void *str2, os_size n)
{
    os_uint32 i = 0;
    os_uint8 *s1 = (os_uint8 *)str1;
    os_uint8 *s2 = (os_uint8 *)str2;
    while (i < n)
    {
        if (s1[i] != s2[i])
        {
            return s1[i] - s2[i];
        }
        i++;
    }
    return 0;
}

void *k__memset(void *str, os_intn c, os_size n)
{
    os_utf8 *s;
    s = (os_utf8 *)str;
    while (n > 0)
    {
        *s = c;
        s++;
        n--;
    }
    return str;
}

os_utf8 *k__strcpy(os_utf8 *dst, const os_utf8 *src)
{
    os_utf8 *dstSave = dst;
    os_intn c;
    do
    {
        c = *dst++ = *src++;
    } while (c);
    return dstSave;
}

os_intn k__strlen(const os_utf8 *string)
{
    const os_utf8 *base = string;
    while (*string++)
        ;
    return string - base - 1;
}

long k__strtol(const os_utf8 *s, os_utf8 **end, os_intn base)
{
    os_intn i;
    os_size ch, value = 0, neg = 0;

    if (s[0] == '-')
    {
        neg = 1;
        ++s;
    }
    if (s[0] == '0' && s[1] == 'x')
    {
        base = 16;
        s += 2;
    }
    for (i = 0; i <= 8; ++i)
    {
        ch = *s++;
        if ('0' <= ch && ch <= '9')
            ch -= '0';
        else if ('A' <= ch && ch < base - 10 + 'A')
            ch = ch - 'A' + 10;
        else if ('a' <= ch && ch < base - 10 + 'a')
            ch = ch - 'a' + 10;
        else
            break;
        value = value * base + ch;
    }
    if (end)
        *end = (os_utf8 *)s - 1;
    if (neg)
        value = -(os_intn)value;
    return value;
}

os_intn k__atoi(const os_utf8 *s)
{
    return k__strtol(s, (os_utf8 **)NULL, 10);
}

os_utf8 *k__itoa(os_intn num, os_utf8 *dst, os_intn base)
{
    os_intn negate = 0;
    os_intn place;
    os_size un;
    os_size digit;
    os_utf8 c;
    os_utf8 text[20];

    un = num;
    if (un == 0)
    {
        k__strcpy(dst, "0");
        return dst;
    }
    if (base == 10 && num < 0)
    {
        un = -num;
        negate = 1;
    }
    text[16] = 0;
    text[15] = '0';
    for (place = 15; place >= 0; --place)
    {
        /*digit = un % base; JML FIXME subc */
        if (base == 10)
        {
            digit = un % 10;
        }
        else
        {
            digit = un & 0x0F;
            digit = un - ((un / 16) * 16);
        }
        /*assert(digit < base);*/
        if (un == 0 && place < 15 && base == 10 && negate)
        {
            c = '-';
            negate = 0;
        }
        else if (digit < 10)
            c = (os_utf8)('0' + digit);
        else if (digit >= base)
            c = 'G';
        else
            c = (os_utf8)('a' + digit - 10);
        text[place] = c;
        if (base == 10)
        {
            un = un / 10;
        }
        else
        {
            un = (un >> 4) & 0x0FFFFFFF;
        }
        /*un = un / ((unsigned var)base);*/
        if (un == 0 && negate == 0)
            break;
        if (place == 0)
            break;
    }
    k__strcpy(dst, text + place);
    return dst;
}

os_intn k__sprintf(os_utf8 *s, const os_utf8 *format,
                   os_intn arg0, os_intn arg1, os_intn arg2, os_intn arg3,
                   os_intn arg4, os_intn arg5, os_intn arg6, os_intn arg7)
{
    os_intn argv[8];
    os_intn argc = 0, width, length;
    os_utf8 f = 0, prev, text[20], fill;

    argv[0] = arg0;
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;
    argv[4] = arg4;
    argv[5] = arg5;
    argv[6] = arg6;
    argv[7] = arg7;

    for (;;)
    {
        prev = f;
        f = *format++;
        if (f == 0)
            return argc;
        else if (f == '%')
        {
            width = 0;
            fill = ' ';
            f = *format++;
            while ('0' <= f && f <= '9')
            {
                width = width * 10 + f - '0';
                f = *format++;
            }
            if (f == '.')
            {
                fill = '0';
                f = *format++;
            }
            if (f == 0)
                return argc;

            if (f == 'd')
            {
                k__memset(s, fill, width);
                k__itoa(argv[argc++], text, 10);
                length = (os_intn)k__strlen(text);
                if (width < length)
                    width = length;
                k__strcpy(s + width - length, text);
            }
            else if (f == 'x' || f == 'f')
            {
                k__memset(s, '0', width);
                k__itoa(argv[argc++], text, 16);
                length = (os_intn)k__strlen(text);
                if (width < length)
                    width = length;
                k__strcpy(s + width - length, text);
            }
            else if (f == 'c')
            {
                *s++ = (os_utf8)argv[argc++];
                *s = 0;
            }
            else if (f == 's')
            {
                length = k__strlen((os_utf8 *)argv[argc]);
                if (width > length)
                {
                    k__memset(s, ' ', width - length);
                    s += width - length;
                }
                k__strcpy(s, (os_utf8 *)argv[argc++]);
            }
            s += k__strlen(s);
        }
        else
        {
            if (f == '\n' && prev != '\r')
                *s++ = '\r';
            *s++ = f;
        }
        *s = 0;
    }
}

void k__printf2(const os_utf8 *format,
                os_intn arg0, os_intn arg1, os_intn arg2, os_intn arg3,
                os_intn arg4, os_intn arg5, os_intn arg6, os_intn arg7)
{
    os_utf8 buffer[128];
    k__sprintf(buffer, format, arg0, arg1, arg2, arg3,
               arg4, arg5, arg6, arg7);
    k__puts(buffer);
}

void k__init()
{

    k__fb_init();
    k__shell_fg = 0xFFFFFF07;
    k__shell_bg = 0xFF000000;
    k__shell_x = 0;
    k__shell_y = 0;
}

void k__draw_char(os_intn x, os_intn y, os_uint32 c)
{
    os_uint8 *d;
    os_int32 i, j;
    os_uint8 *p;
    os_uint8 *pl;
    os_int32 l;

    if (c < ' ' || c > 126)
    {
        return;
    }
    c -= ' ';
    d = (os_uint8 *)&font[16 * c];
    p = (os_uint8 *)k__fb + (y * k__fb_pitch);
    if (k__fb_bpp == 32)
    {
        for (i = 0; i < 16; i++)
        {
            if ((y + i) < 0)
            {
                continue;
            }
            else if ((y + i) >= k__fb_height)
            {
                return;
            }
            l = d[i];
            pl = p + (x * 4);
            for (j = 0; j < 8; j++)
            {
                if ((x + j) < 0)
                {
                }
                else if ((x + j) >= k__fb_width)
                {
                }
                else if (l & 0x80)
                {
                    ((os_int32 *)pl)[0] = k__shell_fg;
                }
                else
                {
                    ((os_int32 *)pl)[0] = k__shell_bg;
                }
                pl += 4;
                l <<= 1;
            }
            p += k__fb_pitch;
        }
    }
    else if (k__fb_bpp == 8)
    {
        for (i = 0; i < 16; i++)
        {
            if ((y + i) < 0)
            {
                continue;
            }
            else if ((y + i) >= k__fb_height)
            {
                return;
            }
            l = d[i];
            pl = p + x;
            for (j = 0; j < 8; j++)
            {
                if ((x + j) < 0)
                {
                }
                else if ((x + j) >= k__fb_width)
                {
                }
                else if (l & 0x80)
                {
                    pl[0] = k__shell_fg;
                }
                else
                {
                    pl[0] = k__shell_bg;
                    pl[0] = 0;
                }
                pl += 1;
                l <<= 1;
            }
            p += k__fb_pitch;
        }
    }
    else
    {
        for (i = 0; i < 16; i++)
        {
            if ((y + i) < 0)
            {
                continue;
            }
            else if ((y + i) >= k__fb_height)
            {
                return;
            }
            l = d[i];
            pl = p + (x * 3);
            for (j = 0; j < 8; j++)
            {
                if ((x + j) < 0)
                {
                }
                else if ((x + j) >= k__fb_width)
                {
                }
                else if (l & 0x80)
                {
                    pl[0] = k__shell_fg;
                    pl[1] = k__shell_fg >> 8;
                    pl[2] = k__shell_fg >> 16;
                }
                else
                {
                    pl[0] = k__shell_bg;
                    pl[1] = k__shell_bg >> 8;
                    pl[2] = k__shell_bg >> 16;
                }
                pl += 3;
                l <<= 1;
            }
            p += k__fb_pitch;
        }
    }
}

void k__scroll(os_intn a)
{
    os_uint8 *src;
    os_int32 i, j;
    os_uint8 *p;
    if (a <= 0)
    {
        return;
    }

    src = (os_uint8 *)k__fb + (a * k__fb_pitch);
    k__memcpy((os_void *)k__fb, src, (k__fb_height - a) * k__fb_pitch);

    for (i = 0; i < a; i++)
    {
        p = (os_uint8 *)k__fb + ((k__fb_height - i - 1) * k__fb_pitch);
        if (k__fb_bpp == 32)
        {
            for (j = 0; j < k__fb_width; j++)
            {
                ((os_int32 *)p)[0] = k__shell_bg;
                p += 4;
            }
        }
        else if (k__fb_bpp == 8)
        {
            for (j = 0; j < k__fb_width; j++)
            {
                p[0] = (os_utf8)k__shell_bg;
                p += 1;
            }
        }
        else
        {
            for (j = 0; j < k__fb_width; j++)
            {
                p[0] = k__shell_bg;
                p[1] = k__shell_bg >> 8;
                p[2] = k__shell_bg >> 16;
                p += 3;
            }
        }
    }
}

void k__putchar(os_int32 ch)
{
    os_int32 n = 0;
    if (ch == '\n')
    {
        k__shell_x = -FONT_WIDTH;
        k__shell_y += FONT_HEIGHT;
    }
    else if (ch == '\b')
    {
        k__shell_x -= FONT_WIDTH;
        if (k__shell_x < 0)
        {
            k__shell_x = 0;
        }
        k__draw_char(k__shell_x, k__shell_y, ' ');
        k__shell_x -= FONT_WIDTH;
    }
    else
    {
        k__draw_char(k__shell_x, k__shell_y, ch);
    }

    k__shell_x += FONT_WIDTH;
    if (k__shell_x >= k__fb_width)
    {
        k__shell_x = 0;
        k__shell_y += FONT_HEIGHT;
    }
    while (k__shell_y > (k__fb_height - FONT_HEIGHT))
    {
        n++;
        k__shell_y -= FONT_HEIGHT;
    }
    if (n)
    {
        k__scroll(FONT_HEIGHT * n);
    }
}

void k__puts(os_utf8 *s)
{
#ifdef __AMD64__
    if (!k__fb)
    {
        uefi__puts(s);
        return;
    }
#endif
    while (*s)
    {
        k__putchar(*s);
#ifdef __RPI400__
        pl011__write(*s);
#endif
        s++;
    }
}
