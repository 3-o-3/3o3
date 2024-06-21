/*
 * Modified MMXXIV by Jean-Marc Lienher
 *
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_GDI_H_
#define NK_GDI_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>

typedef struct GdiFont GdiFont;
NK_API struct nk_context *nk_gdi_init(GdiFont *font, HDC window_dc, unsigned int width, unsigned int height);
NK_API int nk_gdi_handle_event(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
NK_API void nk_gdi_render(struct nk_color clear);
NK_API void nk_gdi_shutdown(void);

/* font */
NK_API GdiFont *nk_gdifont_create(const char *name, int size);
NK_API void nk_gdifont_del(GdiFont *font);
NK_API void nk_gdi_set_font(GdiFont *font);

#endif
// fill configuration
struct giftk_vertex
{
    float pos[2]; // important to keep it to 2 floats
    float uv[2];
    unsigned char col[4];
};

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_GDI_IMPLEMENTATION
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

struct GdiFont
{
    struct nk_user_font nk;
    int height;
    HFONT handle;
    HDC dc;
};

struct GdiTex
{
    char *pixels;
    int w;
    int h;
};

static struct
{
    HBITMAP bitmap;
    HDC window_dc;
    HDC memory_dc;
    unsigned int width;
    unsigned int height;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct GdiTex textures[4];
} gdi;

static void
nk_create_image(struct nk_image *image, const char *frame_buffer, const int width, const int height)
{
    if (image && frame_buffer && (width > 0) && (height > 0))
    {
        const unsigned char *src = (const unsigned char *)frame_buffer;
        INT row = ((width * 3 + 3) & ~3);
        LPBYTE lpBuf, pb = NULL;
        BITMAPINFO bi = {0};
        HBITMAP hbm;
        int v, i;

        image->w = width;
        image->h = height;
        image->region[0] = 0;
        image->region[1] = 0;
        image->region[2] = width;
        image->region[3] = height;

        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = width;
        bi.bmiHeader.biHeight = height;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 24;
        bi.bmiHeader.biCompression = BI_RGB;
        bi.bmiHeader.biSizeImage = row * height;

        hbm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&lpBuf, NULL, 0);

        pb = lpBuf + row * height;
        for (v = 0; v < height; v++)
        {
            pb -= row;
            for (i = 0; i < row; i += 3)
            {
                pb[i + 0] = src[0];
                pb[i + 1] = src[1];
                pb[i + 2] = src[2];
                src += 3;
            }
        }
        SetDIBits(NULL, hbm, 0, height, lpBuf, &bi, DIB_RGB_COLORS);
        image->handle.ptr = hbm;
    }
}

static void
nk_delete_image(struct nk_image *image)
{
    if (image && image->handle.id != 0)
    {
        HBITMAP hbm = (HBITMAP)image->handle.ptr;
        DeleteObject(hbm);
        memset(image, 0, sizeof(struct nk_image));
    }
}

static void
nk_gdi_draw_image(short x, short y, unsigned short w, unsigned short h,
                  struct nk_image img, struct nk_color col)
{
    HBITMAP hbm = (HBITMAP)img.handle.ptr;
    HDC hDCBits;
    BITMAP bitmap;

    if (!gdi.memory_dc || !hbm)
        return;

    hDCBits = CreateCompatibleDC(gdi.memory_dc);
    GetObject(hbm, sizeof(BITMAP), (LPSTR)&bitmap);
    SelectObject(hDCBits, hbm);
    StretchBlt(gdi.memory_dc, x, y, w, h, hDCBits, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
    DeleteDC(hDCBits);
}

static COLORREF
convert_color(struct nk_color c)
{
    return c.r | (c.g << 8) | (c.b << 16);
}

static void
nk_gdi_scissor(HDC dc, float x, float y, float w, float h)
{
    SelectClipRgn(dc, NULL);
    IntersectClipRect(dc, (int)x, (int)y, (int)(x + w + 1), (int)(y + h + 1));
}

struct giftk_bresenham
{
    int da;
    int d;
    int a;
};

struct giftk_edge
{
    int dx;
    int dy;
    int d;
    int x;
    int y;
    int y1;
    struct giftk_bresenham u;
    struct giftk_bresenham v;
};

static void giftk_edge_uv(struct giftk_edge *aa, struct giftk_vertex bb, struct giftk_vertex cc, struct GdiTex *t)
{
    int u0 = (int)(bb.uv[0] * t->w);
    int v0 = (int)(bb.uv[1] * t->h);
    int u1 = (int)(cc.uv[0] * t->w);
    int v1 = (int)(cc.uv[1] * t->h);

    aa->u.da = u1 - u0;
    aa->u.a = u0;

    if (aa->u.da > 0)
    {
        aa->u.d = (aa->u.da << 1) - aa->dy;
    }
    else
    {
        aa->u.d = (aa->u.da << 1) + aa->dy;
    }

    aa->v.da = v1 - v0;
    aa->v.a = v0;
    if (aa->v.da > 0)
    {
        aa->v.d = (aa->v.da << 1) - aa->dy;
    }
    else
    {
        aa->v.d = (aa->v.da << 1) + aa->dy;
    }
}

static void giftk_edge_set(struct giftk_edge *aa, struct giftk_vertex bb, struct giftk_vertex cc, struct GdiTex *t)
{
    int x0 = (int)bb.pos[0];
    int y0 = (int)bb.pos[1];
    int x1 = (int)cc.pos[0];
    int y1 = (int)cc.pos[1];

    int ax1;
    if (y0 < y1)
    {
        ax1 = x1;
        aa->x = x0;
        aa->y = y0;
        aa->y1 = y1;
    }
    else
    {
        ax1 = x0;
        aa->x = x1;
        aa->y = y1;
        aa->y1 = y0;
    }
    aa->dx = ax1 - aa->x;
    aa->dy = aa->y1 - aa->y;

    if (aa->dx > 0)
    {
        aa->d = (aa->dx << 1) - aa->dy;
    }
    else
    {
        aa->d = (aa->dx << 1) + aa->dy;
    }

    if (y0 < y1)
    {
        giftk_edge_uv(aa, bb, cc, t);
    }
    else
    {
        giftk_edge_uv(aa, cc, bb, t);
    }
}

static int doit(int inc, struct giftk_edge *e, struct giftk_bresenham *b)
{
    if (e->dy < 1)
    {
        return b->a;
    }
    if (b->da > 0)
    {
        while (b->d > 0)
        {
            b->a++;
            b->d -= e->dy << 1;
        }
    }
    else if (b->da < 0)
    {
        while (b->d < 0)
        {
            b->a--;
            b->d += e->dy << 1;
        }
    }
    if (inc)
    {
        b->d += b->da << 1;
    }

    return b->a;
}

static void plot(HDC dc, int inc, int x, int y, struct giftk_edge *edges, struct GdiTex tex, unsigned char col[4])
{
    unsigned char *co = tex.pixels;
    struct giftk_edge *e;
    int i;
    int uvx = 0;
    int uvy = 0;

    for (i = 0; i < 3; i++)
    {
        e = edges + i;
        if (e->y <= y && e->y1 >= y)
        {
            uvx = doit(inc, e, &e->u);
            uvy = doit(inc, e, &e->v);
        }
    }
    co += (tex.w * uvy + uvx) * 4;
    COLORREF color = (int)((col[0] * co[3] * co[0] / 510) | ((co[3] * col[1] * co[1] / 510) << 8) | ((col[2] * co[3] * co[2] / 510) << 16));
    if (co[3] > 1)
    {
        SetPixel(dc, x, y, color);
    }
}

static void nk_gdi_render_textured_tri(HDC dc, struct giftk_vertex a, struct giftk_vertex b, struct giftk_vertex c, struct GdiTex tex)
{
    struct nk_color col;
    COLORREF color;
    struct giftk_edge edges[3];
    struct giftk_edge *e;
    int to;
    int from;
    int y, x;
    int i, j, n;

    giftk_edge_set(edges + 0, a, b, &tex);
    giftk_edge_set(edges + 1, b, c, &tex);
    giftk_edge_set(edges + 2, c, a, &tex);

    y = edges[0].y;
    if (edges[1].y < y)
    {
        y = edges[1].y;
    }
    if (edges[2].y < y)
    {
        y = edges[2].y;
    }
    while (edges[0].y1 >= y || edges[1].y1 >= y || edges[2].y1 >= y)
    {
        from = 0x0FFFFFFF;
        to = -0x0FFFFFFF;
        for (i = 0; i < 3; i++)
        {
            e = edges + i;
            if (y >= e->y && y <= e->y1)
            {
                if (e->x < from)
                {
                    from = e->x;
                }
                if (e->x > to)
                {
                    to = e->x;
                }
                if (e->dy > 0)
                {
                    if (e->dx > 0)
                    {
                        while (e->d > 0)
                        {
                            e->x++;
                            e->d -= e->dy << 1;
                        }
                    }
                    else if (e->dx < 0)
                    {
                        while (e->d < 0)
                        {
                            e->x--;
                            e->d += e->dy << 1;
                        }
                    }
                    e->d += e->dx << 1;
                }
            }
        }
        if (tex.w > 0 && tex.h > 0)
        {
            x = from;
            plot(dc, 1, x, y, edges, tex, a.col);
            for (x = from + 1; x < to; x++)
            {
                plot(dc, 0, x, y, edges, tex, a.col);
            }
        }
        else
        {
            // col = nk_green;
            col = nk_rgba(a.col[0], a.col[1], a.col[2], a.col[3]);
            color = convert_color(col);
            SetDCPenColor(dc, color);
            MoveToEx(dc, from, y, NULL);
            LineTo(dc, to, y);
        }
        y++;
    }
}

static void
nk_gdi_render_triangles(HDC dc, const int tex, const struct giftk_vertex *pnts, const nk_draw_index *offset, int count, int stride)
{
    int i = 0;
    int j = 0;
    POINT points[3];
    struct nk_color col;
    COLORREF color;
    for (i = 0; i < count; ++i)
    {
        if (tex > 0)
        {
            struct GdiTex u = {0};
            struct giftk_vertex *p = (void *)((char *)pnts);

            nk_gdi_render_textured_tri(dc, p[offset[i]], p[offset[i + 1]], p[offset[i + 2]], gdi.textures[1]);
            // printf("TEXTURE %p: %p %f %f\n", c, gdi.textures[1].pixels, pnts[offset[i]].uv[0], pnts[offset[i]].uv[1]);
            i += 2;
        }
        else
        {
            struct giftk_vertex *p = (void *)((char *)pnts + stride * offset[i]);
            points[j].x = p[0].pos[0];
            points[j].y = p[0].pos[1];
            j++;
            if (j == 3)
            {
                col = nk_rgba(pnts[offset[i]].col[0], pnts[offset[i]].col[1], pnts[offset[i]].col[2], pnts[offset[i]].col[3]);
                color = convert_color(col);
                SetDCBrushColor(dc, color);
                SetDCPenColor(dc, color);
                Polygon(dc, points, j);
                j = 0;
            }
        }
    }
#undef MAX_POINTS
}

static void
nk_gdi_clear(HDC dc, struct nk_color col)
{
    COLORREF color = convert_color(col);
    RECT rect;
    SetRect(&rect, 0, 0, gdi.width, gdi.height);
    SetBkColor(dc, color);

    ExtTextOutW(dc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

static void
nk_gdi_blit(HDC dc)
{
    BitBlt(dc, 0, 0, gdi.width, gdi.height, gdi.memory_dc, 0, 0, SRCCOPY);
}

GdiFont *
nk_gdifont_create(const char *name, int size)
{
    TEXTMETRICW metric;
    GdiFont *font = (GdiFont *)calloc(1, sizeof(GdiFont));
    if (!font)
        return NULL;
    font->dc = CreateCompatibleDC(0);
    font->handle = CreateFontA(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name);
    SelectObject(font->dc, font->handle);
    GetTextMetricsW(font->dc, &metric);
    font->height = metric.tmHeight;
    return font;
}

static float
nk_gdifont_get_text_width(nk_handle handle, float height, const char *text, int len)
{
    GdiFont *font = (GdiFont *)handle.ptr;
    SIZE size;
    int wsize;
    WCHAR *wstr;
    if (!font || !text)
        return 0;

    wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
    wstr = (WCHAR *)_alloca(wsize * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
    if (GetTextExtentPoint32W(font->dc, wstr, wsize, &size))
        return (float)size.cx;
    return -1.0f;
}

void nk_gdifont_del(GdiFont *font)
{
    if (!font)
        return;
    DeleteObject(font->handle);
    DeleteDC(font->dc);
    free(font);
}

static void
nk_gdi_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    (void)usr;
    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
    {
        HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
        if (mem)
        {
            SIZE_T size = GlobalSize(mem) - 1;
            if (size)
            {
                LPCWSTR wstr = (LPCWSTR)GlobalLock(mem);
                if (wstr)
                {
                    int utf8size = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)), NULL, 0, NULL, NULL);
                    if (utf8size)
                    {
                        char *utf8 = (char *)malloc(utf8size);
                        if (utf8)
                        {
                            WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)), utf8, utf8size, NULL, NULL);
                            nk_textedit_paste(edit, utf8, utf8size);
                            free(utf8);
                        }
                    }
                    GlobalUnlock(mem);
                }
            }
        }
        CloseClipboard();
    }
}

static void
nk_gdi_clipboard_copy(nk_handle usr, const char *text, int len)
{
    if (OpenClipboard(NULL))
    {
        int wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
        if (wsize)
        {
            HGLOBAL mem = (HGLOBAL)GlobalAlloc(GMEM_MOVEABLE, (wsize + 1) * sizeof(wchar_t));
            if (mem)
            {
                wchar_t *wstr = (wchar_t *)GlobalLock(mem);
                if (wstr)
                {
                    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
                    wstr[wsize] = 0;
                    GlobalUnlock(mem);

                    SetClipboardData(CF_UNICODETEXT, mem);
                }
            }
        }
        CloseClipboard();
    }
}

NK_API struct nk_context *
nk_gdi_init(GdiFont *gdifont, HDC window_dc, unsigned int width, unsigned int height)
{
    int w, h;
    struct nk_font *nfont;
    struct nk_user_font *font = &gdifont->nk;
    struct nk_draw_null_texture tex_null; // fixme
    void *p;
    font->userdata = nk_handle_ptr(gdifont);
    font->height = (float)gdifont->height;
    font->width = nk_gdifont_get_text_width;

    gdi.bitmap = CreateCompatibleBitmap(window_dc, width, height);
    gdi.window_dc = window_dc;
    gdi.memory_dc = CreateCompatibleDC(window_dc);
    gdi.width = width;
    gdi.height = height;
    SelectObject(gdi.memory_dc, gdi.bitmap);

    nk_font_atlas_init_default(&gdi.atlas);
    nk_font_atlas_begin(&gdi.atlas);
    nfont = nk_font_atlas_add_default(&gdi.atlas, 13, 0);

    p = nk_font_atlas_bake(&gdi.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    gdi.textures[1].pixels = malloc(w * h * 4);
    memcpy(gdi.textures[1].pixels, p, w * h * 4);
    gdi.textures[1].w = w;
    gdi.textures[1].h = h;
    nk_font_atlas_end(&gdi.atlas, nk_handle_id(1), &tex_null);

    nk_style_set_font(&gdi.ctx, &nfont->handle);
    nk_init_default(&gdi.ctx, &nfont->handle);
    gdi.ctx.clip.copy = nk_gdi_clipboard_copy;
    gdi.ctx.clip.paste = nk_gdi_clipboard_paste;
    return &gdi.ctx;
}

NK_API void
nk_gdi_set_font(GdiFont *gdifont)
{
    struct nk_user_font *font = &gdifont->nk;
    font->userdata = nk_handle_ptr(gdifont);
    font->height = (float)gdifont->height;
    font->width = nk_gdifont_get_text_width;
    nk_style_set_font(&gdi.ctx, font);
}

NK_API int
nk_gdi_handle_event(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        unsigned width = LOWORD(lparam);
        unsigned height = HIWORD(lparam);
        if (width != gdi.width || height != gdi.height)
        {
            DeleteObject(gdi.bitmap);
            gdi.bitmap = CreateCompatibleBitmap(gdi.window_dc, width, height);
            gdi.width = width;
            gdi.height = height;
            SelectObject(gdi.memory_dc, gdi.bitmap);
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(wnd, &paint);
        nk_gdi_blit(dc);
        EndPaint(wnd, &paint);
        return 1;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        int down = !((lparam >> 31) & 1);
        int ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

        switch (wparam)
        {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            nk_input_key(&gdi.ctx, NK_KEY_SHIFT, down);
            return 1;

        case VK_DELETE:
            nk_input_key(&gdi.ctx, NK_KEY_DEL, down);
            return 1;

        case VK_RETURN:
            nk_input_key(&gdi.ctx, NK_KEY_ENTER, down);
            return 1;

        case VK_TAB:
            nk_input_key(&gdi.ctx, NK_KEY_TAB, down);
            return 1;

        case VK_LEFT:
            if (ctrl)
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else
                nk_input_key(&gdi.ctx, NK_KEY_LEFT, down);
            return 1;

        case VK_RIGHT:
            if (ctrl)
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else
                nk_input_key(&gdi.ctx, NK_KEY_RIGHT, down);
            return 1;

        case VK_BACK:
            nk_input_key(&gdi.ctx, NK_KEY_BACKSPACE, down);
            return 1;

        case VK_HOME:
            nk_input_key(&gdi.ctx, NK_KEY_TEXT_START, down);
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_START, down);
            return 1;

        case VK_END:
            nk_input_key(&gdi.ctx, NK_KEY_TEXT_END, down);
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_END, down);
            return 1;

        case VK_NEXT:
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_DOWN, down);
            return 1;

        case VK_PRIOR:
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_UP, down);
            return 1;

        case 'A':
            if (ctrl)
            {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_SELECT_ALL, down);
                return 1;
            }
            break;

        case 'C':
            if (ctrl)
            {
                nk_input_key(&gdi.ctx, NK_KEY_COPY, down);
                return 1;
            }
            break;

        case 'V':
            if (ctrl)
            {
                nk_input_key(&gdi.ctx, NK_KEY_PASTE, down);
                return 1;
            }
            break;

        case 'X':
            if (ctrl)
            {
                nk_input_key(&gdi.ctx, NK_KEY_CUT, down);
                return 1;
            }
            break;

        case 'Z':
            if (ctrl)
            {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_UNDO, down);
                return 1;
            }
            break;

        case 'R':
            if (ctrl)
            {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_REDO, down);
                return 1;
            }
            break;
        }
        return 0;
    }

    case WM_CHAR:
        if (wparam >= 32)
        {
            nk_input_unicode(&gdi.ctx, (nk_rune)wparam);
            return 1;
        }
        break;

    case WM_LBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_LBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        nk_input_button(&gdi.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_RBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_RBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_MBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_MBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_MOUSEWHEEL:
        nk_input_scroll(&gdi.ctx, nk_vec2(0, (float)(short)HIWORD(wparam) / WHEEL_DELTA));
        return 1;

    case WM_MOUSEMOVE:
        nk_input_motion(&gdi.ctx, (short)LOWORD(lparam), (short)HIWORD(lparam));
        return 1;

    case WM_LBUTTONDBLCLK:
        nk_input_button(&gdi.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        return 1;
    }

    return 0;
}

NK_API void
nk_gdi_shutdown(void)
{
    DeleteObject(gdi.memory_dc);
    DeleteObject(gdi.bitmap);
    nk_free(&gdi.ctx);
}

NK_API void
nk_gdi_render(struct nk_color clear)
{
    const struct nk_draw_command *cmd;
    struct nk_buffer cmds, verts, idx;
    struct nk_convert_config cfg = {0};
    const nk_draw_index *offset = NULL;
    static struct nk_color white = {255, 255, 255, 255};
    struct nk_draw_null_texture tex_null; // fixme
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct giftk_vertex, pos)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct giftk_vertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct giftk_vertex, col)},
        {NK_VERTEX_LAYOUT_END}

    };
    int vs = sizeof(struct giftk_vertex);
    size_t vp = offsetof(struct giftk_vertex, pos);
    size_t vt = offsetof(struct giftk_vertex, uv);
    size_t vc = offsetof(struct giftk_vertex, col);

    HDC memory_dc = gdi.memory_dc;
    SelectObject(memory_dc, GetStockObject(DC_PEN));
    SelectObject(memory_dc, GetStockObject(DC_BRUSH));
    nk_gdi_clear(memory_dc, clear);

    tex_null.texture.ptr = NULL;
    tex_null.uv.x = 0.5f;
    tex_null.uv.y = 0.5f;
    cfg.shape_AA = NK_ANTI_ALIASING_ON;
    cfg.line_AA = NK_ANTI_ALIASING_ON;
    cfg.vertex_layout = vertex_layout;
    cfg.vertex_size = sizeof(struct giftk_vertex);
    cfg.vertex_alignment = NK_ALIGNOF(struct giftk_vertex);
    cfg.circle_segment_count = 22;
    cfg.curve_segment_count = 22;
    cfg.arc_segment_count = 22;
    cfg.global_alpha = 1.0f;
    cfg.tex_null = tex_null;
    //
    // setup buffers and convert

    nk_buffer_init_default(&cmds);
    nk_buffer_init_default(&verts);
    nk_buffer_init_default(&idx);

    nk_convert(&gdi.ctx, &cmds, &verts, &idx, &cfg);

    //
    // draw
    offset = (const nk_draw_index *)nk_buffer_memory_const(&idx);
    nk_draw_foreach(cmd, &gdi.ctx, &cmds)
    {
        struct giftk_vertex *vertices = (struct giftk_vertex *)nk_buffer_memory_const(&verts);
        if (!cmd->elem_count)
            continue;
        nk_gdi_scissor(memory_dc, cmd->clip_rect.x, cmd->clip_rect.y, cmd->clip_rect.w, cmd->clip_rect.h);
        nk_gdi_render_triangles(memory_dc, cmd->texture.id, vertices, offset, cmd->elem_count, vs);
        offset += cmd->elem_count;
    }
    /*
    SDL_RenderGeometryRaw(sdl.renderer,
        (SDL_Texture*)cmd->texture.ptr,
        (const float*)((const nk_byte*)vertices + vp), vs,
        (const SDL_Color*)((const nk_byte*)vertices + vc), vs,
        (const float*)((const nk_byte*)vertices + vt), vs,
        (vbuf.needed / vs),
        (void*)offset, cmd->elem_count, 2);
    */
    nk_buffer_free(&cmds);
    nk_buffer_free(&verts);
    nk_buffer_free(&idx);

    nk_gdi_blit(gdi.window_dc);
    nk_clear(&gdi.ctx);
}

#endif
