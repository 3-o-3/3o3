/******************************************************************************
 *                       OS-3o3 operating system
 *
 *                            growing buffer
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 *
 *****************************************************************************/

#include "../include/buf.h"
#include <stdlib.h>

/**
 * allocate a new biffer
 */
struct buf *buf__new(os_int32 size)
{
    struct buf *self;
    if (size < 8)
    {
        size = 8;
    }
    self = malloc(sizeof(struct buf));
    self->buf = malloc(size);
    self->size = size;
    self->len = 0;
    return self;
}

/**
 * free buffer
 */
void buf__dispose(struct buf *self)
{
    free(self->buf);
    free(self);
}

/**
 * get buffer data
 */
os_utf8 *buf__getstr(struct buf *self)
{
    if (self->len < self->size)
    {
        self->buf[self->len] = 0;
    }
    else
    {
        buf__add8(self, 0);
        self->len--;
    }
    return self->buf;
}

/**
 * append nul terminated string
 */
void buf__addstr(struct buf *self, os_utf8 *data)
{
    while (*data)
    {
        buf__add8(self, *data);
        data++;
    }
}

/**
 * append hexadecimal 8 bit value
 */
void buf__addhex8(struct buf *self, os_int32 data)
{
    os_utf8 *hex;
    hex = "0123456789ABCDEF";

    buf__add8(self, hex[(data >> 4) & 0x0F]);
    buf__add8(self, hex[data & 0x0F]);
}

/**
 * append hexadecimal 16 bit value
 */
void buf__addhex16(struct buf *self, os_int32 data)
{
    buf__addhex8(self, (data >> 8) & 0xFF);
    buf__addhex8(self, data & 0xFF);
}

/**
 * append 32 bit hexadeciaml value
 */
void buf__addhex32(struct buf *self, os_int32 data)
{
    buf__addhex16(self, (data >> 16) & 0xFFFF);
    buf__addhex16(self, data & 0xFFFF);
}

/**
 * append integer decimal value
 */
void buf__addint(struct buf *self, os_int32 data)
{
    os_int32 d;
    os_utf8 b[64];
    os_int32 i;
    os_int32 c;

    i = 0;
    d = data;
    if (data < 0)
    {
        buf__add8(self, '-');
        d = -data;
        if (d == data)
        {
            // d == 0x80000000
            d++; // -2147483648 + 1
            // d == 0x80000001
            d = -d; //  2147483647
            // d == 0x7FFFFFFF
            c = d % 10 + 1; // c == 7 + 1
            data /= 10;
            if (c == 10)
            {
                b[i] = '0';
                data++; // add carry
            }
            else
            {
                b[i] = '0' + c;
            }
            i++;
        }
    }
    while (data > 0)
    {
        b[i] = '0' + (data % 10);
        i++;
        data /= 10;
    }
    while (i > 0)
    {
        i--;
        buf__add8(self, b[i]);
    }
}

/**
 * append a byte
 */
void buf__add8(struct buf *self, os_int32 data)
{
    os_utf8 *n;
    if (self->len >= self->size)
    {
        self->size += 512;
        n = malloc(self->size);
        memcpy(n, self->buf, self->len);
        free(self->buf);
        self->buf = n;
    }
    self->buf[self->len] = data;
    self->len++;
}

/**
 * append 2 bytes
 */
void buf__add16(struct buf *self, os_int32 data)
{
    buf__add8(self, data & 0xFF);
    buf__add8(self, (data >> 8) & 0xFF);
}

/**
 * append 4 bytes
 */
void buf__add32(struct buf *self, os_int32 data)
{
    buf__add16(self, data & 0xFFFF);
    buf__add16(self, (data >> 16) & 0xFFFF);
}

/**
 * read unicode codepoint in UTF-8 string
 * \param b pointer to value to read
 * \param len length of b
 * \param code_point returned code point value
 * \returns byte size of the value
 */
os_int32 buf__utf8tocp(os_utf8 *b, os_int32 len, os_int32 *code_point)
{
    os_int32 u;
    os_int32 i;
    os_int32 v;
    i = 0;
    u = 0;
    v = 0;
    if (len < 1)
    {
        return 0;
    }
    {
        v = b[i];
        if (v < 0x80)
        {
            u = v;
        }
        else if (v < 0xC0)
        {
            // error...
            return -1;
        }
        else if (v < 0xE0)
        {
            if (len < 2)
            {
                return -1;
            }
            u = (v & 0x1F) << 6;
            i++;
            u |= b[i] & 0x3F;
        }
        else if (v < 0xF0)
        {
            if (len < 3)
            {
                return -1;
            }
            u = (v & 0x0F) << 12;
            i++;
            u |= (b[i] & 0x3F) << 6;
            i++;
            u |= b[i] & 0x3F;
        }
        else if (v < 0xF8)
        {
            if (len < 4)
            {
                return -1;
            }
            u = (v & 0x07) << 18;
            i++;
            u |= (b[i] & 0x3F) << 12;
            i++;
            u |= (b[i] & 0x3F) << 6;
            i++;
            u |= b[i] & 0x3F;
        }
        else if (v < 0xFC)
        {
            if (len < 5)
            {
                return -1;
            }
            u = (v & 0x03) << 24;
            i++;
            u |= (b[i] & 0x3F) << 18;
            i++;
            u |= (b[i] & 0x3F) << 12;
            i++;
            u |= (b[i] & 0x3F) << 6;
            i++;
            u |= b[i] & 0x3F;
        }
        else
        {
            if (len < 6)
            {
                return -1;
            }
            u = (v & 0x03) << 30;
            i++;
            u |= (b[i] & 0x3F) << 24;
            i++;
            u |= (b[i] & 0x3F) << 18;
            i++;
            u |= (b[i] & 0x3F) << 12;
            i++;
            u |= (b[i] & 0x3F) << 6;
            i++;
            u |= b[i] & 0x3F;
        }
        i++;
    }
    *code_point = u;
    return i;
}

/**
 * append a unicode codepoint to UTF-8 buffer
 */
void buf__add_utf8(struct buf *self, os_int32 data)
{
    data &= 0xFFFFFFFF;

    if (data < 0)
    {
        buf__add8(self, ((data >> 30) & 3) | 252);  // 2^30
        buf__add8(self, ((data >> 24) & 63) | 128); // 2^24
        buf__add8(self, ((data >> 18) & 63) | 128); // 2^18
        buf__add8(self, ((data >> 12) & 63) | 128); // 2^12
        buf__add8(self, ((data >> 6) & 63) | 128);  // 2^6
        buf__add8(self, ((data) & 63) | 128);
    }
    else if (data < 0x80)
    {
        buf__add8(self, ((data) & 127));
    }
    else if (data < 0x800)
    {
        buf__add8(self, ((data >> 6) & 31) | 192);
        buf__add8(self, ((data) & 63) | 128);
    }
    else if (data < 0x10000)
    {
        buf__add8(self, ((data >> 12) & 15) | 224);
        buf__add8(self, ((data >> 6) & 63) | 128);
        buf__add8(self, ((data) & 63) | 128);
    }
    else if (data < 0x200000)
    {
        buf__add8(self, ((data >> 18) & 7) | 240);
        buf__add8(self, ((data >> 12) & 63) | 128);
        buf__add8(self, ((data >> 6) & 63) | 128);
        buf__add8(self, ((data) & 63) | 128);
    }
    else if (data < 0x4000000)
    {
        buf__add8(self, ((data >> 24) & 3) | 248);
        buf__add8(self, ((data >> 18) & 63) | 128);
        buf__add8(self, ((data >> 12) & 63) | 128);
        buf__add8(self, ((data >> 6) & 63) | 128);
        buf__add8(self, ((data) & 63) | 128);
    }
    else
    {
        buf__add8(self, ((data >> 30) & 3) | 252);  // 2^30
        buf__add8(self, ((data >> 24) & 63) | 128); // 2^24
        buf__add8(self, ((data >> 18) & 63) | 128); // 2^18
        buf__add8(self, ((data >> 12) & 63) | 128); // 2^12
        buf__add8(self, ((data >> 6) & 63) | 128);  // 2^6
        buf__add8(self, ((data) & 63) | 128);
    }
}

/**
 * read a unicode codepoint form UTF-16 buffer
 * \param b UTF-16 pointer to value
 * \param len length in byte of b
 * \param code_point returned codepoint
 * \returns length in bytes of the value in b
 */
os_int32 buf__utf16tocp(os_utf8 *b, os_int32 len, os_int32 *code_point)
{
    os_int32 data;
    os_int32 d2;
    if (len < 2)
    {
        return -1;
    }
    data = b[0] + (b[1] << 8);
    if (data < 0xD800 || (data > 0xDFFF && data < 0x10000))
    {
        *code_point = data;
        return 2;
    }

    if (len < 4)
    {
        *code_point = data;
        return 2;
    }
    data = (data - 0xD800) << 10;
    d2 = b[2] + (b[3] << 8);
    data += d2 - 0xDC00;
}

/**
 * append a UTF-16 value to the buffer
 */
void buf__add_utf16(struct buf *self, os_int32 data)
{
    if (data < 0xD800 || (data > 0xDFFF && data < 0x10000))
    {
        buf__add16(self, data);
        return;
    }

    data -= 0x010000;
    buf__add16(self, ((data & 0xFFC00) >> 10) + 0xD800);
    buf__add16(self, (data & 0x003FF) + 0xDC00);
}
