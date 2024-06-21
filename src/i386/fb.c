/******************************************************************************
 *                        OS-3o3 operating system
 *
 *                          framebuffer for i386
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 *
 *****************************************************************************/

/* https://forum.osdev.org/viewtopic.php?f=2&t=30186 */

#include "../../include/klib.h"

static os_uint8 *gfx_info;
volatile os_uint8 *k__fb;
os_uint8 *fb_front;
os_uint16 k__fb_height;
os_uint16 k__fb_width;
os_uint16 k__fb_pitch;
os_uint16 k__fb_bpp;
os_uint16 k__fb_isrgb;

os_uint8 *get_framebuffer()
{
    return *((os_uint8 **)(&gfx_info[40]));
}

os_intn get_fb_linear()
{
    return gfx_info[0] & 0x80; /*FIXME rgba | bgra  | palette*/
}

os_intn get_fb_width()
{
    return gfx_info[18] | gfx_info[19] << 8;
}

os_intn get_fb_height()
{
    return gfx_info[20] | gfx_info[21] << 8;
}

os_intn get_fb_pitch()
{
    return gfx_info[16] | gfx_info[17] << 8;
}

os_intn get_fb_bpp()
{
    return gfx_info[25];
}

void k__fb_init()
{
    gfx_info = (os_uint8 *)GFX_INFO;
    fb_front = get_framebuffer();
    fb_front = (os_uint8 *)(*((os_uint32 **)(&gfx_info[40])));
    k__fb = fb_front;
    k__fb_width = get_fb_width();
    k__fb_height = get_fb_height();
    k__fb_pitch = get_fb_pitch();
    k__fb_bpp = get_fb_bpp();
}

void fb_swap()
{
}
