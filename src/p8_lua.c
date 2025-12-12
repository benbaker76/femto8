/*
 * p8_lua.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

extern "C" {
#include <assert.h>
#include "p8_symbols.h"
#include "pico_font.h"
#include "p8_audio.h"
#include "p8_emu.h"
#include "p8_lua.h"
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>    // GetProcessMemoryInfo
#elif defined(__GLIBC__)
#include <malloc.h>   // mallinfo / mallinfo2
#else
#include <unistd.h>   // sysconf
#include <stdio.h>    // fopen/fscanf
#endif
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "p8_lua_helper.h"
#include "p8_print_helper.h"
#include "pico_font.h"
#include "p8_parser.h"
}
#include "lua_api.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

lua_State *L = NULL;

const void *m_lua_init = NULL;
const void *m_lua_update = NULL;
const void *m_lua_update60 = NULL;
const void *m_lua_draw = NULL;

char m_str_buffer[256] = {0};

static int m_tline_precision = 13;

void lua_load_api();
void lua_shutdown_api();
void lua_print_error(const char *where);
void lua_init_script(const char *script);
void lua_call_function(const char *name, int ret);
void lua_update();
void lua_draw();
void lua_init();

void lua_register_functions(lua_State *L);

static unsigned addr_remap(unsigned address)
{
    if (address >= 0x0000 && address < 0x2000)
        address = (m_memory[MEMORY_SPRITE_PHYS] << 8) | (address & 0x1fff);
    else if (address >= 0x6000 && address < 0x8000)
        address = (m_memory[MEMORY_SCREEN_PHYS] << 8) | (address & 0x1fff);
    return address;
}

// ****************************************************************
// *** Graphics ***
// ****************************************************************

// camera([x,] [y])
int camera(lua_State *L)
{
    int cx = lua_gettop(L) >= 1 ? lua_tointeger(L, 1) : 0;
    int cy = lua_gettop(L) >= 2 ? lua_tointeger(L, 2) : 0;

    camera_set(cx, cy);

    return 0;
}

// circ(x, y, [r,] [col])
int circ(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    int r = lua_gettop(L) >= 3 ? lua_tointeger(L, 3) : 4;
    int col = lua_gettop(L) >= 4 ? lua_tointeger(L, 4) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 4 ? (lua_tonumber(L, 4).bits() & 0xffff) : 0;

    draw_circ(x, y, r, col, fillp);

    return 0;
}

// circfill(x, y, [r,] [col])
int circfill(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    int r = lua_gettop(L) >= 3 ? lua_tointeger(L, 3) : 4;
    int col = lua_gettop(L) >= 4 ? lua_tointeger(L, 4) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 4 ? (lua_tonumber(L, 4).bits() & 0xffff) : 0;

    draw_circfill(x, y, r, col, fillp);

    return 0;
}

// clip(x, y, w, h)
int clip(lua_State *L)
{
    if (lua_gettop(L) == 0)
        clip_set(0, 0, P8_WIDTH, P8_HEIGHT);
    else
    {
        int x0 = lua_tointeger(L, 1);
        int y0 = lua_tointeger(L, 2);
        int w = lua_tointeger(L, 3);
        int h = lua_tointeger(L, 4);
        bool clip_previous = lua_gettop(L) >= 5 ? lua_toboolean(L, 5) : false;

        int prev_x0 = 0, prev_y0 = 0, prev_x1 = P8_WIDTH, prev_y1 = P8_HEIGHT;
        if (clip_previous)
            clip_get(&prev_x0, &prev_y0, &prev_x1, &prev_y1);

        int x1 = x0 + w;
        int y1 = y0 + h;
        x0 = MAX(x0, prev_x0);
        y0 = MAX(y0, prev_y0);
        x1 = MIN(x1, prev_x1);
        y1 = MIN(y1, prev_y1);

        clip_set(x0, y0, x1-x0, y1-y0);
    }

    return 0;
}

// cls([color])
int cls(lua_State *L)
{
    int color = lua_gettop(L) == 1 ? lua_tointeger(L, -1) : 0;

    clear_screen(color);

    return 0;
}

// color(col)
int color(lua_State *L)
{
    int col = lua_gettop(L) == 1 ? lua_tointeger(L, 1) : 6;

    pencolor_set(col);

    return 0;
}

// cursor([x,] [y,] [col])
int cursor(lua_State *L)
{
    int x = lua_gettop(L) >= 1 ? lua_tointeger(L, 1) : 0;
    int y = lua_gettop(L) >= 2 ? lua_tointeger(L, 2) : 0;
    int color = lua_gettop(L) >= 3 ? lua_tointeger(L, 3) : -1;

    cursor_set(x, y, (color == -1) ? -1 : ((pencolor_get() & 0xf0) | color));
    left_margin_set(x);

    return 0;
}

// fget(n, [f])
int fget(lua_State *L)
{
    int n = lua_tointeger(L, 1);
    uint8_t flags = m_memory[MEMORY_SPRITEFLAGS + n];

    if (lua_gettop(L) == 2)
    {
        int f = lua_tointeger(L, 2);
        lua_pushboolean(L, (flags & (1 << f)) != 0);
    }
    else
    {
        lua_pushinteger(L, flags);
    }

    return 1;
}

// fillp([pat])
int fillp(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        m_memory[MEMORY_FILLP] = 0;
        m_memory[MEMORY_FILLP + 1] = 0;
        m_memory[MEMORY_FILLP_ATTR] = 0;
    } else {
        uint32_t n = lua_tonumber(L, 1).bits();
        m_memory[MEMORY_FILLP] = (n >> 16) & 0xff;
        m_memory[MEMORY_FILLP + 1] = (n >> 24) & 0xff;
        m_memory[MEMORY_FILLP_ATTR] = ((n & 0x8000) ? 1 : 0) | ((n & 0x4000) ? 2 : 0) | ((n & 0x2000) ? 4: 0);

    }
    return 0;
}

// flip()
int flip(lua_State *L)
{
    p8_flip();

    return 0;
}

// fset(n, [f,] v)
int fset(lua_State *L)
{
    int n = lua_tointeger(L, 1);
    assert(n >= 0 && n <= 255);

    if (lua_gettop(L) == 3)
    {
        int f = lua_tointeger(L, 2);
        bool v = lua_toboolean(L, 3);
        assert(f >= 0 && f <= 7);

        if (v)
            m_memory[MEMORY_SPRITEFLAGS + n] |= (1 << f);
        else
            m_memory[MEMORY_SPRITEFLAGS + n] &= ~(1 << f);
    }
    else
    {
        int f = lua_tointeger(L, 2);
        assert(f >= 0 && f <= 255);
        m_memory[MEMORY_SPRITEFLAGS + n] = f;
    }

    return 0;
}

// line([x0,] [y0,] x1, y1, [col])
int line(lua_State *L)
{
    int x0, y0, x1, y1, col, fillp, valid;

    if (lua_gettop(L) == 0) {
        m_memory[MEMORY_LINE_VALID] = 1;
    } else {
        if (lua_gettop(L) >= 4) {
            valid = 1;
            x0 = lua_tointeger(L, 1);
            y0 = lua_tointeger(L, 2);
            x1 = lua_tointeger(L, 3);
            y1 = lua_tointeger(L, 4);
            col = lua_gettop(L) == 5 ? lua_tointeger(L, 5) : pencolor_get() & 0xF;
            fillp = lua_gettop(L) >= 5 ? (lua_tonumber(L, 5).bits() & 0xffff) : 0;
        } else {
            valid = !m_memory[MEMORY_LINE_VALID];
            x0 = m_memory[MEMORY_LINE_X] | (m_memory[MEMORY_LINE_X + 1] << 8);
            y0 = m_memory[MEMORY_LINE_Y] | (m_memory[MEMORY_LINE_Y + 1] << 8);
            x1 = lua_tointeger(L, 1);
            y1 = lua_tointeger(L, 2);
            col = lua_gettop(L) == 3 ? lua_tointeger(L, 3) : pencolor_get() & 0xF;
            fillp = lua_gettop(L) >= 3 ? (lua_tonumber(L, 3).bits() & 0xffff) : 0;
        }

        if (valid)
            draw_line(x0, y0, x1, y1, col, fillp);

        m_memory[MEMORY_LINE_X] = x1 & 0xff;
        m_memory[MEMORY_LINE_X + 1] = (x1 >> 8) & 0xff;
        m_memory[MEMORY_LINE_Y] = y1 & 0xff;
        m_memory[MEMORY_LINE_Y + 1] = (y1 >> 8) & 0xff;
        m_memory[MEMORY_LINE_VALID] = 0;
    }

    return 0;
}

// oval(x0, y0, x1, y1, [col])
int oval(lua_State *L)
{
    int x0 = lua_tointeger(L, 1);
    int y0 = lua_tointeger(L, 2);
    int x1 = lua_tointeger(L, 3);
    int y1 = lua_tointeger(L, 4);
    int col = lua_gettop(L) >= 5 ? lua_tointeger(L, 5) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 5 ? (lua_tonumber(L, 5).bits() & 0xffff) : 0;

    draw_oval(x0, y0, x1, y1, col, fillp);

    return 0;
}

// ovalfill(x0, y0, x1, y1, [col])
int ovalfill(lua_State *L)
{
    int x0 = lua_tointeger(L, 1);
    int y0 = lua_tointeger(L, 2);
    int x1 = lua_tointeger(L, 3);
    int y1 = lua_tointeger(L, 4);
    int col = lua_gettop(L) >= 5 ? lua_tointeger(L, 5) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 5 ? (lua_tonumber(L, 5).bits() & 0xffff) : 0;

    draw_ovalfill(x0, y0, x1, y1, col, fillp);

    return 0;
}

// pal(c0, c1, [p])
// pal(tbl, [p])
int pal(lua_State *L)
{
    if (lua_gettop(L) == 0)
    {
        reset_color();
    }
    else if (lua_istable(L, 1))
    {
        int p = lua_gettop(L) == 2 ? lua_tointeger(L, 2) : PALTYPE_DRAW;
        lua_pushnil(L);
        while (lua_next(L, 1))
        {
            int c0 = lua_tointeger(L, -2);
            int c1 = lua_tointeger(L, -1);
            color_set(p, c0, c1);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    else
    {
        int c0 = lua_tointeger(L, 1);
        int c1 = lua_tointeger(L, 2);
        int p = lua_gettop(L) == 3 ? lua_tointeger(L, 3) : PALTYPE_DRAW;

        color_set(p, c0, c1);
    }

    return 0;
}

// palt([col,] [t])
int palt(lua_State *L)
{
    if (lua_gettop(L) == 0)
    {
        reset_color();
    }
    else if (lua_gettop(L) == 1)
    {
        int b = lua_tounsigned(L, 1);
        for (int col=0;col<16;++col) {
            int t = (b >> (15-col)) & 1;
            uint8_t c = color_get(PALTYPE_DRAW, col);

            color_set(PALTYPE_DRAW, col, t ? (c | 0x10) : (c & 0xF));
        }
    }
    else
    {
        int col = lua_tointeger(L, 1);
        int t = lua_toboolean(L, 2);
        uint8_t c = color_get(PALTYPE_DRAW, col);

        color_set(PALTYPE_DRAW, col, t ? (c | 0x10) : (c & 0xF));
    }

    return 0;
}

// pget(x, y)
int pget(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    int x0, y0, x1, y1;
    clip_get(&x0, &y0, &x1, &y1);

    if (x >= x0 && y >= y0 && x < x1 && y < y1) {
        lua_pushinteger(L, gfx_get(x, y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE));
    } else {
        int default_val = (m_memory[MEMORY_MISCFLAGS] & 0x10) ? m_memory[MEMORY_PGET_DEFAULT] : 0;
        lua_pushinteger(L, default_val);
    }

    return 1;
}

// print(str, [x,] [y,] [col])
int print(lua_State *L)
{
    size_t len;
    const char *str = lua_tolstring(L, 1, &len);
    int right;

    if (lua_gettop(L) >= 1 && lua_gettop(L) <= 2)
    {
        int x, y;
        cursor_get(&x, &y);
        int col = lua_gettop(L) == 2 ? lua_tointeger(L, 2) : pencolor_get() & 0xF;
        if (lua_gettop(L) == 2)
            pencolor_set(col);
        draw_text(str, len, x, y, col, left_margin_get(), (m_memory[MEMORY_MISCFLAGS] & 0x4) == 0, &x, &y, &right);
        cursor_set(x, y, -1);
        if ((m_memory[MEMORY_MISCFLAGS] & 0x40) == 0)
            scroll();
    }
    else if (lua_gettop(L) >= 3)
    {
        int x = lua_tointeger(L, 2);
        int y = lua_tointeger(L, 3);
        int col = lua_gettop(L) == 4 ? lua_tointeger(L, 4) : pencolor_get() & 0xF;
        if (lua_gettop(L) == 4)
            pencolor_set(col);

        left_margin_set(x);

        draw_text(str, len, x, y, col, x, false, NULL, NULL, &right);
    }
    else
        assert(false);

    lua_pushinteger(L, right);
    return 1;
}

// pset(x, y, [c])
int pset(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    int c = lua_gettop(L) == 3 ? lua_tointeger(L, 3) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 3 ? (lua_tonumber(L, 3).bits() & 0xffff) : 0;
    pixel_set(x, y, c, fillp, DRAWTYPE_GRAPHIC);

    return 0;
}

// rect(x0, y0, x1, y1, [col])
int rect(lua_State *L)
{
    int x0 = lua_tointeger(L, 1);
    int y0 = lua_tointeger(L, 2);
    int x1 = lua_tointeger(L, 3);
    int y1 = lua_tointeger(L, 4);
    int col = lua_gettop(L) == 5 ? lua_tointeger(L, 5) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 5 ? (lua_tonumber(L, 5).bits() & 0xffff) : 0;

    int left = MIN(x0, x1);
    int top = MIN(y0, y1);
    int right = MAX(x0, x1);
    int bottom = MAX(y0, y1);

    draw_rect(left, top, right, bottom, col, fillp);

    return 0;
}

// rectfill(x0, y0, x1, y1, [col])
int rectfill(lua_State *L)
{
    int x0 = lua_tointeger(L, 1);
    int y0 = lua_tointeger(L, 2);
    int x1 = lua_tointeger(L, 3);
    int y1 = lua_tointeger(L, 4);
    int col = lua_gettop(L) >= 5 ? lua_tointeger(L, 5) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 5 ? (lua_tonumber(L, 5).bits() & 0xffff) : 0;

    int left = MIN(x0, x1);
    int top = MIN(y0, y1);
    int right = MAX(x0, x1);
    int bottom = MAX(y0, y1);

    draw_rectfill(left, top, right, bottom, col, fillp);

    return 0;
}

// rrect(x, y, w, h, r, [col])
int rrect(lua_State *L)
{
    int left = lua_tointeger(L, 1);
    int top = lua_tointeger(L, 2);
    int w = lua_tointeger(L, 3);
    int h = lua_tointeger(L, 4);
    int r = lua_gettop(L) >= 5 ? lua_tointeger(L, 5) : 0;
    int col = lua_gettop(L) >= 6 ? lua_tointeger(L, 6) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 6 ? (lua_tonumber(L, 6).bits() & 0xffff) : 0;

    int right = left + w-1;
    int bottom = top + h-1;
    r = MAX(0, MIN(r, MIN(w, h) / 2));

    draw_hline(left+r, top, right-r, col, fillp);
    draw_hline(left+r, bottom, right-r, col, fillp);
    draw_vline(left, top+r, bottom-r, col, fillp);
    draw_vline(right, top+r, bottom-r, col, fillp);
    draw_circ_mask(left+r, top+r, r, col, fillp, 1 << 3 | 1 << 7);
    draw_circ_mask(right-r, top+r, r, col, fillp, 1 << 2 | 1 << 6);
    draw_circ_mask(left+r, bottom-r, r, col, fillp, 1 << 1 | 1 << 5);
    draw_circ_mask(right-r, bottom-r, r, col, fillp, 1 << 0 | 1 << 4);

    return 0;
}

// rrectfill(x, y, w, h, r, [col])
int rrectfill(lua_State *L)
{
    int left = lua_tointeger(L, 1);
    int top = lua_tointeger(L, 2);
    int w = lua_tointeger(L, 3);
    int h = lua_tointeger(L, 4);
    int r = lua_gettop(L) >= 5 ? lua_tointeger(L, 5) : 0;
    int col = lua_gettop(L) >= 6 ? lua_tointeger(L, 6) : pencolor_get() & 0xF;
    int fillp = lua_gettop(L) >= 6 ? (lua_tonumber(L, 6).bits() & 0xffff) : 0;

    int right = left + w;
    int bottom = top + h;
    r = MAX(0, MIN(r, MIN(w, h) / 2));

    draw_rectfill(left, top+r, right, bottom-r, col, fillp);
    draw_rectfill(left+r, top, right-r, top+r, col, fillp);
    draw_rectfill(left+r, bottom-r, right-r, bottom, col, fillp);
    draw_circfill_mask(left+r, top+r, r, col, fillp, 1 << 3 | 1 << 7);
    draw_circfill_mask(right-r, top+r, r, col, fillp, 1 << 2 | 1 << 6);
    draw_circfill_mask(left+r, bottom-r, r, col, fillp, 1 << 1 | 1 << 5);
    draw_circfill_mask(right-r, bottom-r, r, col, fillp, 1 << 0 | 1 << 4);

    return 0;
}

// sget(x, y)
int sget(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    if (x >= 0 && y >= 0 && x < P8_WIDTH && y < P8_HEIGHT) {
        lua_pushinteger(L, gfx_get(x, y, MEMORY_SPRITES, MEMORY_SPRITES_SIZE));
    } else {
        int default_val = (m_memory[MEMORY_MISCFLAGS] & 0x10) ? m_memory[MEMORY_SGET_DEFAULT] : 0;
        lua_pushinteger(L, default_val);
    }

    return 1;
}

// spr(n, x, y, [w,] [h,] [flip_x,] [flip_y])
int spr(lua_State *L)
{
    assert(lua_isnumber(L, 2) && lua_isnumber(L, 3));

    int n = lua_tointeger(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    int w = 1;
    int h = 1;
    bool flip_x = false, flip_y = false;

    if (lua_gettop(L) > 3)
    {
        assert(lua_gettop(L) >= 5);

        w = lua_tointeger(L, 4);
        h = lua_tointeger(L, 5);

        if (lua_gettop(L) >= 6)
            flip_x = lua_toboolean(L, 6);

        if (lua_gettop(L) >= 7)
            flip_y = lua_toboolean(L, 7);
    }

    draw_sprites(n, x, y, w, h, flip_x, flip_y);

    return 0;
}

// sset(x, y, [c])
int sset(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);
    int c = lua_gettop(L) >= 3 ? lua_tointeger(L, 3) : pencolor_get() & 0xF;

    if (x >= 0 && y >= 0 && x < P8_WIDTH && y < P8_HEIGHT)
        gfx_set(x, y, MEMORY_SPRITES, MEMORY_SPRITES_SIZE, c);

    return 0;
}

// sspr(sx, sy, sw, sh, dx, dy, [dw,] [dh,] [flip_x,] [flip_y])
int sspr(lua_State *L)
{
    int sx = lua_tointeger(L, 1);
    int sy = lua_tointeger(L, 2);
    int sw = lua_tointeger(L, 3);
    int sh = lua_tointeger(L, 4);
    int dx = lua_tointeger(L, 5);
    int dy = lua_tointeger(L, 6);
    int dw = lua_to_or_default(L, integer, 7, sw);
    int dh = lua_to_or_default(L, integer, 8, sh);
    bool flip_x = lua_to_or_default(L, boolean, 9, false);
    bool flip_y = lua_to_or_default(L, boolean, 10, false);
    float scale_x = (float)dw / sw;
    float scale_y = (float)dh / sh;

    draw_scaled_sprite(sx, sy, sw, sh, dx, dy, scale_x, scale_y, flip_x, flip_y);

    return 0;
}

// tline( x0, y0, x1, y1, mx, my, [mdx,] [mdy])
int tline(lua_State *L)
{
    int x0 = lua_tointeger(L, 1);
    int y0 = lua_tointeger(L, 2);
    int x1 = lua_tointeger(L, 3);
    int y1 = lua_tointeger(L, 4);
    lua_Number mx = lua_tonumber(L, 5);
    lua_Number my = lua_tonumber(L, 6);
    lua_Number mdx = lua_to_or_default(L, number, 7, lua_Number(1) / lua_Number(8));
    lua_Number mdy = lua_to_or_default(L, number, 8, lua_Number(0));
    int layer = lua_to_or_default(L, integer, 9, 0);

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
        mx &= (lua_Number(m_memory[MEMORY_TLINE_MASK_X]) - lua_Number::frombits(0x1));
        my &= (lua_Number(m_memory[MEMORY_TLINE_MASK_Y]) - lua_Number::frombits(0x1));
        int tx = (mx.bits() >> m_tline_precision) + m_memory[MEMORY_TLINE_OFFSET_X] * 8;
        int ty = (my.bits() >> m_tline_precision) + m_memory[MEMORY_TLINE_OFFSET_Y] * 8;
        int index = map_get(tx / 8, ty / 8);
        uint8_t sprite_flags = m_memory[MEMORY_SPRITEFLAGS + index];
        if (index != 0 && (layer == 0 || ((layer & sprite_flags) == layer))) {
            int sx = (index & 0xF) * SPRITE_WIDTH;
            int sy = (index >> 4) * SPRITE_HEIGHT;
            int col = gfx_get(sx + (tx % 8), sy + (ty % 8), MEMORY_SPRITES, MEMORY_SPRITES_SIZE);
            pixel_set(x0, y0, col, 0, DRAWTYPE_DEFAULT);
        }

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;

            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }

        mx += mdx;
        my += mdy;
    }

    return 0;
}

// ****************************************************************
// *** Tables ***
// ****************************************************************

// add(tbl, v)
// all(tbl)
// count(tbl)
// del(tbl, v)
// foreach(tbl, func)
// pairs(tbl)

// ****************************************************************
// *** Input ***
// ****************************************************************

// btn([i,] [p])
int btn(lua_State *L)
{
    // button
    if (lua_gettop(L) >= 1)
    {
        int i = lua_tointeger(L, 1);
        int p = lua_gettop(L) >= 2 ? lua_tointeger(L, 2) : 0;

        if (p >= PLAYER_COUNT)
            p = 0;

        if (i < BUTTON_COUNT)
            lua_pushboolean(L, is_button_set(p, i, false));
        else
            lua_pushboolean(L, false);
    }
    // mask
    else
    {
        lua_pushinteger(L, m_buttons[0] | (m_buttons[1] << 8));
    }

    return 1;
}

// btnp([i,] [p])
int btnp(lua_State *L)
{
    // button
    if (lua_gettop(L) >= 1)
    {
        int i = lua_tointeger(L, 1);
        int p = lua_gettop(L) >= 2 ? lua_tointeger(L, 2) : 0;

        if (p >= PLAYER_COUNT)
            p = 0;

        lua_pushboolean(L, is_button_set(p, i, true));
    }
    // mask
    else
    {
        lua_pushinteger(L, m_buttonsp[0] | (m_buttons[1] << 8));
    }

    return 1;
}

// ****************************************************************
// *** Sound ***
// ****************************************************************

// music(n, [fadems,] [channelmask])
int music(lua_State *L)
{
#ifdef ENABLE_AUDIO
    int n = lua_tointeger(L, 1);
    int fadems = lua_to_or_default(L, integer, 2, 1);
    int channelmask = lua_to_or_default(L, integer, 3, 0);

    audio_music(n, fadems, channelmask);
#endif

    return 0;
}

// sfx(n, [channel,] [offset,] [length])
int sfx(lua_State *L)
{
#ifdef ENABLE_AUDIO
    int n = lua_tointeger(L, 1);
    int channel = lua_to_or_default(L, integer, 2, -1);
    int offset = lua_to_or_default(L, integer, 3, 0);
    int length = lua_to_or_default(L, integer, 3, 32);

    audio_sound(n, channel, offset, m_memory[MEMORY_SFX + length * 64]);
#endif

    return 0;
}

// ****************************************************************
// *** Map ***
// ****************************************************************

// map(celx, cely, sx, sy, celw, celh, [layer])
int map(lua_State *L)
{
    int celx = lua_tointeger(L, 1);
    int cely = lua_tointeger(L, 2);
    int sx = lua_tointeger(L, 3);
    int sy = lua_tointeger(L, 4);
    int celw = lua_gettop(L) >= 5 ? lua_tointeger(L, 5) : P8_WIDTH / SPRITE_WIDTH;
    int celh = lua_gettop(L) >= 6 ? lua_tointeger(L, 6) : P8_HEIGHT / SPRITE_HEIGHT;
    int layer = lua_gettop(L) >= 7 ? lua_tointeger(L, 7) : 0;

    for (int y = 0; y < celh; y++)
    {
        for (int x = 0; x < celw; x++)
        {
            uint8_t index = map_get(celx + x, cely + y);
            uint8_t sprite_flags = m_memory[MEMORY_SPRITEFLAGS + index];

            if (index != 0 && (layer == 0 || ((layer & sprite_flags) == layer)))
            {
                int left = sx + x * SPRITE_WIDTH;
                int top = sy + y * SPRITE_HEIGHT;
                draw_sprite(index, left, top, false, false);
            }
        }
    }

    return 0;
}

// mget(celx, cely)
int mget(lua_State *L)
{
    int celx = lua_tointeger(L, 1);
    int cely = lua_tointeger(L, 2);

    int address = map_cell_addr(celx, cely);
    if (address == 0) {
        int default_val = (m_memory[MEMORY_MISCFLAGS] & 0x10) ? m_memory[MEMORY_MGET_DEFAULT] : 0;
        lua_pushinteger(L, default_val);
    } else {
        lua_pushinteger(L, m_memory[address]);
    }

    return 1;
}

// mset(celx, cely, snum)
int mset(lua_State *L)
{
    int celx = lua_tointeger(L, 1);
    int cely = lua_tointeger(L, 2);
    int snum = lua_tointeger(L, 3);

    map_set(celx, cely, snum);

    return 0;
}

// ****************************************************************
// *** Memory ***
// ****************************************************************

// cstore(destaddr, sourceaddr, len, [filename])
// memcpy(destaddr, sourceaddr, len)
int _memcpy(lua_State *L)
{
    unsigned destaddr = lua_tounsigned(L, 1);
    unsigned sourceaddr = lua_tounsigned(L, 2);
    unsigned len = lua_tounsigned(L, 3);

    if (len < 1 || destaddr == sourceaddr)
        return 0;

    if (sourceaddr < 0 || (sourceaddr + len) > (1 << 16))
        return 0;

    if (destaddr < 0 || (destaddr + len) > (1 << 16))
        return 0;

    while (len > 0) {
        unsigned chunk = MIN(MIN(len, 0x2000 - (sourceaddr & 0x1fff)), 0x2000 - (destaddr & 0x1fff));
        unsigned destaddr1 = addr_remap(destaddr);
        unsigned sourceaddr1 = addr_remap(sourceaddr);
        memmove(m_memory + destaddr1, m_memory + sourceaddr1, chunk);
        destaddr += chunk;
        sourceaddr += chunk;
        len -= chunk;
    }

    return 0;
}

// memset(destaddr, val, len)
int _memset(lua_State *L)
{
    unsigned destaddr = lua_tounsigned(L, 1);
    int val = lua_tointeger(L, 2);
    unsigned len = lua_tounsigned(L, 3);

    while (len > 0) {
        unsigned chunk = MIN(len, 0x2000 - (destaddr & 0x1fff));
        unsigned destaddr1 = addr_remap(destaddr);
        memset(m_memory + destaddr1, val, chunk);
        destaddr += chunk;
        len -= chunk;
    }

    return 0;
}

// peek(addr, [n])
int peek(lua_State *L)
{
    unsigned addr = lua_tounsigned(L, 1);
    unsigned n = lua_gettop(L) >= 2 ? lua_tounsigned(L, 2) : 1;

    addr = addr_remap(addr);

    for (unsigned i=0;i<n;++i)
        lua_pushinteger(L, m_memory[addr+i]);

    return n;
}

// peek2(addr, [n])
int peek2(lua_State *L)
{
    unsigned addr = lua_tounsigned(L, 1);
    unsigned n = lua_gettop(L) >= 2 ? lua_tounsigned(L, 2) : 1;

    addr = addr_remap(addr);

    for (unsigned i=0;i<n;++i)
        lua_pushinteger(L, (m_memory[addr + i*2 + 1] << 8) | (m_memory[addr + i*2]));

    return n;
}

// peek4(addr, [n])
int peek4(lua_State *L)
{
    unsigned addr = lua_tounsigned(L, 1);
    unsigned n = lua_gettop(L) >= 2 ? lua_tounsigned(L, 2) : 1;

    addr = addr_remap(addr);

    for (unsigned i=0;i<n;++i)
        lua_pushnumber(L, z8::fix32::frombits((m_memory[addr + i*4 + 3] << 24) | (m_memory[addr + i*4 + 2] << 16) | (m_memory[addr + i*4 + 1] << 8) | m_memory[addr + i*4]));

    return n;
}

// poke(addr, val1, ...)
int poke(lua_State *L)
{
    unsigned addr = lua_tounsigned(L, 1);

    addr = addr_remap(addr);

    for (int i=2;i<=lua_gettop(L);++i) {
        unsigned val = lua_tounsigned(L, i);

        m_memory[addr + i-2] = val;
    }

    if (addr >= MEMORY_CARTDATA && addr + 1 <= MEMORY_CARTDATA + MEMORY_CARTDATA_SIZE)
        p8_delayed_flush_cartdata();

    return 0;
}

// poke2(addr, val1, ...)
int poke2(lua_State *L)
{
    unsigned addr = lua_tounsigned(L, 1);

    addr = addr_remap(addr);

    for (int i=2;i<=lua_gettop(L);++i) {
        unsigned val = lua_tounsigned(L, i);

        m_memory[addr + (i-2)*2] = val;
        m_memory[addr + (i-2)*2 + 1] = val >> 8;
    }

    if (addr >= MEMORY_CARTDATA && addr + 2 <= MEMORY_CARTDATA + MEMORY_CARTDATA_SIZE)
        p8_delayed_flush_cartdata();

    return 0;
}

// poke4(addr, val1, ...)
int poke4(lua_State *L)
{
    unsigned addr = lua_tounsigned(L, 1);

    addr = addr_remap(addr);

    for (int i=2;i<=lua_gettop(L);++i) {
        uint32_t val = lua_tonumber(L, i).bits();

        m_memory[addr + (i-2)*4] = val;
        m_memory[addr + (i-2)*4 + 1] = val >> 8;
        m_memory[addr + (i-2)*4 + 2] = val >> 16;
        m_memory[addr + (i-2)*4 + 3] = val >> 24;
    }

    if (addr >= MEMORY_CARTDATA && addr + 4 <= MEMORY_CARTDATA + MEMORY_CARTDATA_SIZE)
        p8_delayed_flush_cartdata();

    return 0;
}

// reload(destaddr, sourceaddr, len, [filename])
int reload(lua_State *L)
{
    unsigned destaddr = lua_tounsigned(L, 1);
    unsigned srcaddr = lua_tounsigned(L, 2);
    unsigned len = lua_tounsigned(L, 3);
    destaddr = addr_remap(destaddr);
    const char *file_name = lua_gettop(L) >= 4 ? lua_tostring(L, 4) : NULL;
    uint8_t *src_mem = NULL;
    if (file_name != NULL) {
        uint8_t *buffer = NULL;
        src_mem = (uint8_t *)malloc(CART_MEMORY_SIZE);
        parse_cart_file(file_name, src_mem, NULL, &buffer);
        free(buffer);
    } else {
        src_mem = m_cart_memory;
    }
    if (destaddr >= 0 && destaddr + len <= CART_MEMORY_SIZE && srcaddr >= 0 && srcaddr + len <= CART_MEMORY_SIZE)
        memcpy(m_memory + destaddr, src_mem + srcaddr, len);
    if (file_name != NULL)
        free(src_mem);
    return 0;
}

// ****************************************************************
// *** Math ***
// ****************************************************************

// rnd(max)
int rnd(lua_State *L)
{
    int is_table = 0;

    if (lua_gettop(L) >= 1)
    {
        if (lua_istable(L, 1))
        {
            is_table = 1;
        }
    }

    uint32_t hi = m_memory[MEMORY_RNG_STATE] | (m_memory[MEMORY_RNG_STATE + 1] << 8) |
                  (m_memory[MEMORY_RNG_STATE + 2] << 16) | (m_memory[MEMORY_RNG_STATE + 3] << 24);
    uint32_t lo = m_memory[MEMORY_RNG_STATE + 4] | (m_memory[MEMORY_RNG_STATE + 5] << 8) |
                  (m_memory[MEMORY_RNG_STATE + 6] << 16) | (m_memory[MEMORY_RNG_STATE + 7] << 24);

    hi = (hi << 16) | (hi >> 16);
    hi += lo;
    lo += hi;

    m_memory[MEMORY_RNG_STATE] = hi & 0xFF;
    m_memory[MEMORY_RNG_STATE + 1] = (hi >> 8) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 2] = (hi >> 16) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 3] = (hi >> 24) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 4] = lo & 0xFF;
    m_memory[MEMORY_RNG_STATE + 5] = (lo >> 8) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 6] = (lo >> 16) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 7] = (lo >> 24) & 0xFF;

    if (is_table)
    {
        size_t len = lua_rawlen(L, 1);
        if (len > 0)
        {
            int index = ((hi >> 16) % len) + 1;
            lua_rawgeti(L, 1, index);
        }
        else
        {
            lua_pushnil(L);
        }
    }
    else
    {
        uint32_t max_fixed = (lua_gettop(L) >= 1) ? lua_tonumber(L, 1).bits() : 0x10000;
        uint32_t result_fixed = hi % max_fixed;
        lua_pushnumber(L, z8::fix32::frombits(result_fixed));
    }

    return 1;
}

// srand(val)
int _srand(lua_State *L)
{
    unsigned n = lua_tounsigned(L, 1);

    p8_seed_rng_state(n);

    return 0;
}

// ****************************************************************
// *** Cartridge data ***
// ****************************************************************

// cartdata(id)
int cartdata(lua_State *L)
{
    const char *id = lua_tostring(L, 1);
    bool success = p8_open_cartdata(id);
    lua_pushboolean(L, success);
    return 1;
}

// dget(index)
int dget(lua_State *L)
{
    unsigned index = lua_tounsigned(L, 1);

    lua_pushnumber(L, z8::fix32::frombits((m_memory[MEMORY_CARTDATA + index*4 + 3] << 24) | (m_memory[MEMORY_CARTDATA + index*4 + 2] << 16) | (m_memory[MEMORY_CARTDATA + index*4 + 1] << 8) | m_memory[MEMORY_CARTDATA + index*4]));

    return 1;
}

// dset(index, value)
int dset(lua_State *L)
{
    unsigned index = lua_tounsigned(L, 1);
    uint32_t value = lua_tonumber(L, 2).bits();

    m_memory[MEMORY_CARTDATA + index*4] = value;
    m_memory[MEMORY_CARTDATA + index*4 + 1] = value >> 8;
    m_memory[MEMORY_CARTDATA + index*4 + 2] = value >> 16;
    m_memory[MEMORY_CARTDATA + index*4 + 3] = value >> 24;

    p8_delayed_flush_cartdata();

    return 0;
}

// ****************************************************************
// *** Coroutines ***
// ****************************************************************

// cocreate(func)
// coresume(cor)
// costatus(cor)
// yield()

// ****************************************************************
// *** Values and objects ***
// ****************************************************************

// setmetatable(tbl, metatbl)
// getmetatable(tbl)
// type(v)
// sub(str, from, [to])
int sub(lua_State *L)
{
    const char *str = lua_tostring(L, 1);
    int start = lua_tointeger(L, 2);
    int end = lua_to_or_default(L, integer, 3, -1);

    if (end == -1)
    {
        strncpy(m_str_buffer, str + start - 1, strlen(str) - start + 1);

        lua_pushstring(L, m_str_buffer);

        return 1;
    }

    strncpy(m_str_buffer, str + start - 1, end - start + 1);

    lua_pushstring(L, m_str_buffer);

    return 1;
}

// ****************************************************************
// *** Time ***
// ****************************************************************

// time()
int _time(lua_State *L)
{
    lua_pushnumber(L, (float)m_frames / (float)m_fps);

    return 1;
}

// ****************************************************************
// *** System ***
// ****************************************************************

// menuitem(index, [label, callback])
int menuitem(lua_State *L)
{
    return 0;
}

// extcmd(cmd)

int reset(lua_State *L)
{
    p8_reset();

    return 0;
}

int __attribute__ ((noreturn)) run(lua_State *L)
{
    p8_restart();
}

// ****************************************************************
// *** Debugging ***
// ****************************************************************

// assert(cond, [message])
// printh(str, [filename], [overwrite])
int printh(lua_State *L)
{
    const char *str = lua_tostring(L, 1);

    printf("%s\r\n", str);

    return 0;
}

// stat(n)
int _stat(lua_State *L)
{
    int n = lua_tointeger(L, 1);

    switch (n)
    {
case STAT_MEM_USAGE: {
    double kb = 0.0;

#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             (PROCESS_MEMORY_COUNTERS*)&pmc,
                             sizeof(pmc))) {
        // PrivateUsage = committed private bytes
        kb = (double)pmc.PrivateUsage / 1024.0;
    }

#elif defined(__GLIBC__)
  #if defined(__GLIBC_PREREQ) && __GLIBC_PREREQ(2,33)
    struct mallinfo2 mi = mallinfo2();
    kb = (double)mi.uordblks / 1024.0;
  #else
    struct mallinfo mi = mallinfo();
    kb = (double)mi.uordblks / 1024.0;
  #endif

#else
    // Fallback for Linux/non-glibc: /proc/self/statm
    long pages = 0;
    FILE* f = fopen("/proc/self/statm", "r");
    if (f && fscanf(f, "%*ld %ld", &pages) == 1) {
        fclose(f);
        long page_sz = sysconf(_SC_PAGESIZE);
        kb = (double)pages * (double)page_sz / 1024.0;
    } else if (f) {
        fclose(f);
    }
#endif

    lua_pushnumber(L, (lua_Number)kb);
    break;
    }
    case STAT_CPU_USAGE:
    case STAT_SYSTEM_CPU_USAGE: {
        unsigned elapsed_time = p8_elapsed_time();
        const unsigned target_frame_time = 1000 / m_fps;
        float f = (float)elapsed_time / (float)target_frame_time;
        lua_pushnumber(L, f);
        break;
    }
    case STAT_PARAM:
        lua_pushstring(L, "");
        break;
    case STAT_FRAMERATE:
        lua_pushinteger(L, m_actual_fps);
        break;
    case STAT_TARGET_FRAMERATE:
        lua_pushinteger(L, m_fps);
        break;
    case STAT_RAW_KEYBOARD: {
        int key = lua_tointeger(L, 2);
        lua_pushboolean(L, (key < NUM_SCANCODES) ? m_scancodes[key] : false);
        break;
    }
    case STAT_KEY_PRESSED:
        lua_pushboolean(L, m_keypress != 0);
        break;
    case STAT_KEY_NAME: {
        char s[2] = {(char)m_keypress, '\0'};
        lua_pushstring(L, s);
        m_keypress = 0;
        break;
    }
    case STAT_MOUSE_X:
        lua_pushinteger(L, m_mouse_x);
        break;
    case STAT_MOUSE_Y:
        lua_pushinteger(L, m_mouse_y);
        break;
    case STAT_MOUSE_BUTTONS:
        lua_pushinteger(L, m_mouse_buttons);
        break;
    case STAT_MOUSE_XREL:
        lua_pushinteger(L, m_mouse_xrel);
        break;
    case STAT_MOUSE_YREL:
        lua_pushinteger(L, m_mouse_yrel);
        break;
    case STAT_YEAR:
    case STAT_MONTH:
    case STAT_DAY:
    case STAT_HOUR:
    case STAT_MINUTE:
    case STAT_SECOND: {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        switch (n)
        {
        case STAT_YEAR:
            lua_pushinteger(L, tm->tm_year + 1900);
            break;
        case STAT_MONTH:
            lua_pushinteger(L, tm->tm_mon + 1);
            break;
        case STAT_DAY:
            lua_pushinteger(L, tm->tm_mday);
            break;
        case STAT_HOUR:
            lua_pushinteger(L, tm->tm_hour);
            break;
        case STAT_MINUTE:
            lua_pushinteger(L, tm->tm_min);
            break;
        case STAT_SECOND:
            lua_pushinteger(L, tm->tm_sec);
            break;
        }
        break;
    }
    default:
        if ((n >= 46 && n <= 56) || (n >= 16 && n <= 26)) {
            lua_pushinteger(L, audio_stat(n));
        } else {
            lua_pushinteger(L, 0);
        }
        break;
    }

    return 1;
}

// stop() (undocumented)
// trace() (undocumented)

// ****************************************************************
// *** Misc ***
// ****************************************************************

// ****************************************************************
// ****************************************************************

int warning(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);

    printf("WARNING: %s\r\n", msg);

    return 0;
}

int set_fps(lua_State *L)
{
    int fps = lua_tointeger(L, 1);
    m_fps = MAX(0, fps);

    return 0;
}

int get_mouse_x(lua_State *L)
{
    lua_pushinteger(L, m_mouse_x);

    return 1;
}

int get_mouse_y(lua_State *L)
{
    lua_pushinteger(L, m_mouse_y);

    return 1;
}

int note_to_hz(lua_State *L)
{
    int note = lua_tointeger(L, 1);

    lua_pushinteger(L, 440 * 2 ^ ((note - 33 / 12)));

    return 1;
}

void lua_register_functions(lua_State *L)
{
    // ****************************************************************
    // *** Graphics ***
    // ****************************************************************
    lua_register(L, "camera", camera);
    lua_register(L, "circ", circ);
    lua_register(L, "circfill", circfill);
    lua_register(L, "clip", clip);
    lua_register(L, "cls", cls);
    lua_register(L, "color", color);
    lua_register(L, "cursor", cursor);
    lua_register(L, "fget", fget);
    lua_register(L, "fillp", fillp);
    lua_register(L, "flip", flip);
    lua_register(L, "fset", fset);
    lua_register(L, "line", line);
    lua_register(L, "oval", oval);
    lua_register(L, "ovalfill", ovalfill);
    lua_register(L, "pal", pal);
    lua_register(L, "palt", palt);
    lua_register(L, "pget", pget);
    lua_register(L, "print", print);
    lua_register(L, "pset", pset);
    lua_register(L, "rect", rect);
    lua_register(L, "rectfill", rectfill);
    lua_register(L, "rrect", rrect);
    lua_register(L, "rrectfill", rrectfill);
    lua_register(L, "sget", sget);
    lua_register(L, "spr", spr);
    lua_register(L, "sset", sset);
    lua_register(L, "sspr", sspr);
    lua_register(L, "tline", tline);
    // ****************************************************************
    // *** Tables ***
    // ****************************************************************
    // lua_register(L, "add", add);
    // lua_register(L, "all", all);
    // lua_register(L, "count", count);
    // lua_register(L, "del", del);
    // lua_register(L, "foreach", foreach);
    // lua_register(L, "pairs", pairs);
    // ****************************************************************
    // *** Input ***
    // ****************************************************************
    lua_register(L, "btn", btn);
    lua_register(L, "btnp", btnp);
    // ****************************************************************
    // *** Sound ***
    // ****************************************************************
    lua_register(L, "music", music);
    lua_register(L, "sfx", sfx);
    // ****************************************************************
    // *** Map ***
    // ****************************************************************
    lua_register(L, "map", map);
    lua_register(L, "mget", mget);
    lua_register(L, "mset", mset);
    // ****************************************************************
    // *** Memory ***
    // ****************************************************************
    // lua_register(L, "cstore", cstore);
    lua_register(L, "memcpy", _memcpy);
    lua_register(L, "memset", _memset);
    lua_register(L, "peek", peek);
    lua_register(L, "peek2", peek2);
    lua_register(L, "peek4", peek4);
    lua_register(L, "poke", poke);
    lua_register(L, "poke2", poke2);
    lua_register(L, "poke4", poke4);
    lua_register(L, "reload", reload);
    // ****************************************************************
    // *** Math ***
    // ****************************************************************
    // lua_register(L, "abs", _abs); // in lpico8lib.c
    // lua_register(L, "atan2", _atan2); // in lpico8lib.c
    // lua_register(L, "band", band); // in lpico8lib.c
    // lua_register(L, "bnot", bnot); // in lpico8lib.c
    // lua_register(L, "bor", bor); // in lpico8lib.c
    // lua_register(L, "bxor", bxor); // in lpico8lib.c
    // lua_register(L, "ceil", _ceil); // in lpico8lib.c
    // lua_register(L, "cos", _cos); // in lpico8lib.c
    // lua_register(L, "flr", flr); // in lpico8lib.c
    // lua_register(L, "lshr", lshr); // in lpico8lib.c
    // lua_register(L, "max", max); // in lpico8lib.c
    // lua_register(L, "mid", mid); // in lpico8lib.c
    // lua_register(L, "min", min); // in lpico8lib.c
    lua_register(L, "rnd", rnd);
    // lua_register(L, "rotl", rotl); // in lpico8lib.c
    // lua_register(L, "rotr", rotr); // in lpico8lib.c
    // lua_register(L, "sgn", sgn); // in lpico8lib.c
    // lua_register(L, "shl", shl); // in lpico8lib.c
    // lua_register(L, "shr", shr); // in lpico8lib.c
    // lua_register(L, "sin", _sin); // in lpico8lib.c
    // lua_register(L, "sqrt", _sqrt); // in lpico8lib.c
    lua_register(L, "srand", _srand);
    // ****************************************************************
    // *** Cartridge data ***
    // ****************************************************************
    lua_register(L, "cartdata", cartdata);
    lua_register(L, "dget", dget);
    lua_register(L, "dset", dset);
    // ****************************************************************
    // *** Coroutines ***
    // ****************************************************************
    // lua_register(L, "cocreate", cocreate);
    // lua_register(L, "coresume", coresume);
    // lua_register(L, "costatus", costatus);
    // lua_register(L, "yield", yield);
    // ****************************************************************
    // *** Values and objects ***
    // ****************************************************************
    // lua_register(L, "chr", chr); // in lpico8lib.c
    // lua_register(L, "ord", ord); // in lpico8lib.c
    // lua_register(L, "setmetatable", setmetatable);
    // lua_register(L, "getmetatable", getmetatable);
    // lua_register(L, "type", type);
    lua_register(L, "sub", sub);
    // lua_register(L, "tonum", tonum);
    // lua_register(L, "tostr", tostr); // in lpico8lib.c
    // ****************************************************************
    // *** Time ***
    // ****************************************************************
    lua_register(L, "t", _time);
    lua_register(L, "time", _time);
    // ****************************************************************
    // *** System ***
    // ****************************************************************
    lua_register(L, "menuitem", menuitem);
    // lua_register(L, "extcmd", extcmd);
    lua_register(L, "reset", reset);
    lua_register(L, "run", run);
    // ****************************************************************
    // *** Debugging ***
    // ****************************************************************
    // lua_register(L, "assert", assert);
    lua_register(L, "printh", printh);
    lua_register(L, "stat", _stat);
    // lua_register(L, "stop", stop);
    // lua_register(L, "trace, trace);
    // ****************************************************************
    // *** Misc ***
    // ****************************************************************
}

void lua_load_api()
{
    if (!L)
    {
        L = luaL_newstate();
    }

    luaL_openlibs(L);

    printf("Loading extended PICO-8 Api\r\n");

    if (luaL_dostring(L, lua_api_string))
        lua_print_error("Error loading extended PICO-8 Api");

    lua_setpico8memory(L, m_memory);
}

void lua_shutdown_api()
{
    if (L) {
        lua_close(L);
        L = NULL;
    }
}

void lua_print_error(const char *where)
{
    // for (int i = 1; i < lua_gettop(L); ++i)
    {
        printf("Error on %s\r\n", where);

        // luaL_traceback(L, L, NULL, 1);
        // printf("%s\n", lua_tostring(L, -1));

        if (lua_isstring(L, -1))
        {
            const char *message = lua_tostring(L, -1);
            printf("%s\r\n", message);
        }
    }
    getchar();
}

void lua_init_script(const char *script)
{
    if (!L)
        L = luaL_newstate();

    lua_register_functions(L);

    if (luaL_loadstring(L, script))
        lua_print_error("luaL_loadString");

    int error = lua_pcall(L, 0, 0, 0);

    if (error)
        lua_print_error("lua_pcall on init");

    lua_getglobal(L, "_update");

    if (lua_isfunction(L, -1))
    {
        m_lua_update = lua_topointer(L, -1);
        m_fps = 30;
        lua_pop(L, 1);
    }

    lua_getglobal(L, "_update60");

    if (lua_isfunction(L, -1))
    {
        m_lua_update60 = lua_topointer(L, -1);
        m_fps = 60;
        lua_pop(L, 1);
    }

    lua_getglobal(L, "_draw");

    if (lua_isfunction(L, -1))
    {
        m_lua_draw = lua_topointer(L, -1);
        lua_pop(L, 1);
    }

    lua_getglobal(L, "_init");

    if (lua_isfunction(L, -1))
    {
        m_lua_init = lua_topointer(L, -1);
        lua_pop(L, 1);
    }
}

void lua_call_function(const char *name, int ret)
{
    lua_getglobal(L, name);
    int error = lua_pcall(L, 0, ret, 0);

    if (error)
        lua_print_error(name);
}

void lua_update()
{
    if (m_lua_update60)
        lua_call_function("_update60", 0);
    else if (m_lua_update)
        lua_call_function("_update", 0);
}

void lua_draw()
{
    if (m_lua_draw)
        lua_call_function("_draw", 0);
}

bool lua_has_main_loop_callbacks()
{
    return (m_lua_update != NULL || m_lua_update60 != NULL) && (m_lua_draw != NULL);
}

void lua_init()
{
    if (m_lua_init)
        lua_call_function("_init", 0);
}
