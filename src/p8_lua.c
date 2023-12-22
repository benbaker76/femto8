/*
 * p8_lua.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <assert.h>
#include "p8_symbols.h"
#include "lua_api.h"
#include "pico_font.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "p8_audio.h"
#include "p8_emu.h"
#include "p8_lua.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "pico_font.h"

lua_State *L = NULL;

const void *m_lua_init = NULL;
const void *m_lua_update = NULL;
const void *m_lua_update60 = NULL;
const void *m_lua_draw = NULL;

char m_str_buffer[256] = {0};

void lua_load_api();
void lua_shutdown_api();
void lua_print_error(const char *where);
void lua_init_script(const char *script);
void lua_call_function(const char *name, int ret);
void lua_update();
void lua_draw();
void lua_init();

void lua_register_functions(lua_State *L);

// ****************************************************************
// *** Graphics ***
// ****************************************************************

// camera([x,] [y])
int camera(lua_State *L)
{
    int16_t cx = lua_gettop(L) >= 1 ? lua_tonumber(L, 1) : 0;
    int16_t cy = lua_gettop(L) == 2 ? lua_tonumber(L, 2) : 0;

    camera_set(cx, cy);

    return 0;
}

// circ(x, y, [r,] [col])
int circ(lua_State *L)
{
    int x = lua_tonumber(L, 1);
    int y = lua_tonumber(L, 2);
    int r = lua_gettop(L) >= 3 ? lua_tonumber(L, 3) : 4;
    int col = lua_gettop(L) >= 4 ? lua_tonumber(L, 4) : pencolor_get() & 0xF;

    draw_circ(x, y, r, col);

    return 0;
}

// circfill(x, y, [r,] [col])
int circfill(lua_State *L)
{
    int x = lua_tonumber(L, 1);
    int y = lua_tonumber(L, 2);
    int r = lua_gettop(L) >= 3 ? lua_tonumber(L, 3) : 4;
    int col = lua_gettop(L) >= 4 ? lua_tonumber(L, 4) : pencolor_get() & 0xF;

    draw_circfill(x, y, r, col);

    return 0;
}

// clip(x, y, w, h)
int clip(lua_State *L)
{
    if (lua_gettop(L) == 0)
        clip_set(0, 0, P8_WIDTH - 1, P8_HEIGHT - 1);
    else
    {
        uint8_t x0 = lua_tonumber(L, 1);
        uint8_t y0 = lua_tonumber(L, 2);
        uint8_t w = lua_tonumber(L, 3);
        uint8_t h = lua_tonumber(L, 4);

        clip_set(x0, y0, MIN(x0 + w, P8_WIDTH - 1), MIN(y0 + h, P8_HEIGHT - 1));
    }

    return 0;
}

// cls([color])
int cls(lua_State *L)
{
    clear_screen();

    return 0;
}

// color(col)
int color(lua_State *L)
{
    int col = lua_tonumber(L, 1);

    pencolor_set(col);

    return 0;
}

// cursor([x,] [y,] [col])
int cursor(lua_State *L)
{
    if (lua_gettop(L) >= 2)
    {
        int x = lua_tonumber(L, 1);
        int y = lua_tonumber(L, 2);

        if (lua_gettop(L) == 3)
        {
            int color = lua_tonumber(L, 3);

            cursor_set(x, y, (pencolor_get() & 0xf0) | color);
        }
        else
            cursor_set(x, y, -1);
    }
    else
        cursor_set(0, 0, -1);

    return 0;
}

// fget(n, [f])
int fget(lua_State *L)
{
    int n = lua_tonumber(L, 1);
    uint8_t flags = m_memory[MEMORY_SPRITEFLAGS + n];

    if (lua_gettop(L) == 2)
    {
        int f = lua_tonumber(L, 2);
        lua_pushboolean(L, (flags & (1 << f)) != 0);
    }
    else
    {
        lua_pushnumber(L, flags);
    }

    return 1;
}

// fillp([pat])
int fillp(lua_State *L)
{
    return 0;
}

// flip()
int flip(lua_State *L)
{
    // p8_render();

    return 0;
}

// fset(n, [f,] v)
int fset(lua_State *L)
{
    int n = lua_tonumber(L, 1);

    if (lua_gettop(L) == 3)
    {
        int f = lua_tonumber(L, 2);
        bool v = lua_toboolean(L, 3);
        assert(n >= 0 && n <= 7);

        if (v)
            m_memory[MEMORY_SPRITEFLAGS + n] |= (1 << f);
        else
            m_memory[MEMORY_SPRITEFLAGS + n] &= ~(1 << f);
    }
    else
    {
        int f = lua_tonumber(L, 2);
        m_memory[MEMORY_SPRITEFLAGS + n] = f;
    }

    return 0;
}

// line([x0,] [y0,] x1, y1, [col])
int line(lua_State *L)
{
    int x0 = lua_tonumber(L, 1);
    int y0 = lua_tonumber(L, 2);
    int x1 = lua_tonumber(L, 3);
    int y1 = lua_tonumber(L, 4);
    int col = lua_gettop(L) == 5 ? lua_tonumber(L, 5) : pencolor_get() & 0xF;

    draw_line(x0, y0, x1, y1, col);

    return 0;
}

// pal(c0, c1, [p])
int pal(lua_State *L)
{
    if (lua_gettop(L) == 0)
    {
        reset_color();
    }
    else
    {
        int c0 = lua_tonumber(L, 1);
        int c1 = lua_tonumber(L, 2);
        int p = lua_gettop(L) == 3 ? lua_tonumber(L, 3) : PALTYPE_DRAW;

        color_set(p, c0, c1);
    }

    return 0;
}

// palt([col,] [t])
int palt(lua_State *L)
{
    if (lua_gettop(L) == 0)
    {
        for (int i = 0; i < 16; i++)
        {
            color_set(PALTYPE_DRAW, i, i & 0xF);
            color_set(PALTYPE_SCREEN, i, i & 0xF);
        }

        color_set(PALTYPE_DRAW, 0, 0x10);
    }
    else
    {
        int col = lua_tonumber(L, 1);
        int t = lua_toboolean(L, 2);
        uint8_t c = color_get(PALTYPE_DRAW, col);

        color_set(PALTYPE_DRAW, col, t ? (c | 0x10) : (c & 0xF));
    }

    return 0;
}

// pget(x, y)
int pget(lua_State *L)
{
    int x = lua_tonumber(L, 1);
    int y = lua_tonumber(L, 2);

    lua_pushinteger(L, gfx_get(x, y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE));

    return 1;
}

// print(str, [x,] [y,] [col])
int print(lua_State *L)
{
    const char *str = lua_tostring(L, 1);

    if (lua_gettop(L) == 1)
    {
        int x, y;
        cursor_get(&x, &y);
        int col = pencolor_get() & 0xF;
        int left = draw_text(str, x, y, col);
        cursor_set(x + left, y + GLYPH_HEIGHT, -1);
    }
    else if (lua_gettop(L) >= 3)
    {
        int x = lua_tonumber(L, 2);
        int y = lua_tonumber(L, 3);
        int col = lua_gettop(L) == 4 ? lua_tonumber(L, 4) : pencolor_get() & 0xF;

        draw_text(str, x, y, col);
    }
    else
        assert(false);

    return 0;
}

// pset(x, y, [c])
int pset(lua_State *L)
{
    int x = lua_tonumber(L, 1);
    int y = lua_tonumber(L, 2);
    int c = lua_gettop(L) == 3 ? lua_tonumber(L, 3) : pencolor_get() & 0xF;
    pixel_set(x, y, c);

    return 0;
}

// rect(x0, y0, x1, y1, [col])
int rect(lua_State *L)
{
    int x0 = lua_tonumber(L, 1);
    int y0 = lua_tonumber(L, 2);
    int x1 = lua_tonumber(L, 3);
    int y1 = lua_tonumber(L, 4);
    int col = lua_gettop(L) == 5 ? lua_tonumber(L, 5) : pencolor_get() & 0xF;

    int left = MIN(x0, x1);
    int top = MIN(y0, y1);
    int right = MAX(x0, x1) + 1;
    int bottom = MAX(y0, y1) + 1;

    for (int y = top; y < bottom; y++)
        for (int x = left; x < right; x++)
            if (x == left || y == top || x == right - 1 || y == bottom - 1)
                pixel_set(x, y, col);

    return 0;
}

// rectfill(x0, y0, x1, y1, [col])
int rectfill(lua_State *L)
{
    int x0 = lua_tonumber(L, 1);
    int y0 = lua_tonumber(L, 2);
    int x1 = lua_tonumber(L, 3);
    int y1 = lua_tonumber(L, 4);
    int col = lua_gettop(L) >= 5 ? lua_tonumber(L, 5) : pencolor_get() & 0xF;

    int left = MIN(x0, x1);
    int top = MIN(y0, y1);
    int right = MAX(x0, x1) + 1;
    int bottom = MAX(y0, y1) + 1;

    for (int y = top; y < bottom; y++)
        for (int x = left; x < right; x++)
            pixel_set(x, y, col);

    return 0;
}

// sget(x, y)
int sget(lua_State *L)
{
    int x = lua_tonumber(L, 1);
    int y = lua_tonumber(L, 2);

    lua_pushnumber(L, gfx_get(x, y, MEMORY_SPRITES, MEMORY_SPRITES_SIZE));

    return 1;
}

// spr(n, x, y, [w,] [h,] [flip_x,] [flip_y])
int spr(lua_State *L)
{
    assert(lua_isnumber(L, 2) && lua_isnumber(L, 3));

    int n = lua_tonumber(L, 1);
    int x = lua_tonumber(L, 2);
    int y = lua_tonumber(L, 3);
    float w = 1.0f;
    float h = 1.0f;
    bool flip_x = false, flip_y = false;

    if (lua_gettop(L) > 3)
    {
        assert(lua_gettop(L) >= 5);

        w = lua_tonumber(L, 4);
        h = lua_tonumber(L, 5);

        if (lua_gettop(L) >= 6)
            flip_x = lua_toboolean(L, 6);

        if (lua_gettop(L) >= 7)
            flip_y = lua_toboolean(L, 7);
    }

    for (int sy = 0; sy < h; sy++)
    {
        for (int sx = 0; sx < w; sx++)
        {
            int index = (n == -1.0f ? 0 : n) + sx + sy * 16;
            int left = x + sx * 8;
            int top = y + sy * 8;
            draw_sprite(index, left, top, flip_x, flip_y);
        }
    }

    return 0;
}

// sset(x, y, [c])
int sset(lua_State *L)
{
    int x = lua_tonumber(L, 1);
    int y = lua_tonumber(L, 2);
    int c = lua_gettop(L) >= 3 ? lua_tonumber(L, 3) : pencolor_get() & 0xF;

    gfx_set(x, y, MEMORY_SPRITES, MEMORY_SPRITES_SIZE, c);

    return 0;
}

// sspr(sx, sy, sw, sh, dx, dy, [dw,] [dh,] [flip_x,] [flip_y])
int sspr(lua_State *L)
{
    int sx = lua_tonumber(L, 1);
    int sy = lua_tonumber(L, 2);
    int sw = lua_tonumber(L, 3);
    int sh = lua_tonumber(L, 4);
    int dx = lua_tonumber(L, 5);
    int dy = lua_tonumber(L, 6);
    int dw = lua_to_or_default(L, number, 7, sw);
    int dh = lua_to_or_default(L, number, 8, sh);
    bool flip_x = lua_to_or_default(L, boolean, 9, false);
    bool flip_y = lua_to_or_default(L, boolean, 10, false);
    float scale_x = (float)dw / sw;
    float scale_y = (float)dh / sh;

    draw_scaled_sprite(sx, sy, sw, sh, dx, dy, scale_x, scale_y, flip_x, flip_y);

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
    int p = lua_gettop(L) >= 2 ? lua_tonumber(L, 2) : 0;

    if (p >= PLAYER_COUNT)
        p = 0;

    // button
    if (lua_gettop(L) >= 1)
    {
        int i = lua_tonumber(L, 1);

        if (i < BUTTON_COUNT)
            lua_pushboolean(L, is_button_set(p, i, false));
        else
            lua_pushboolean(L, false);
    }
    // mask
    else
    {
        lua_pushnumber(L, m_buttons[p]);
    }

    return 1;
}

// btnp([i,] [p])
int btnp(lua_State *L)
{
    int p = lua_gettop(L) >= 2 ? lua_tonumber(L, 2) : 0;

    // button
    if (lua_gettop(L) >= 1)
    {
        int i = lua_tonumber(L, 1);
        lua_pushboolean(L, is_button_set(p, i, true));
    }
    // mask
    else
    {
        lua_pushnumber(L, m_prev_buttons[p]);
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
    int n = lua_tonumber(L, 1);
    int fadems = lua_to_or_default(L, number, 2, 1);
    int channelmask = lua_to_or_default(L, number, 3, 0);

    audio_music(n, fadems, channelmask);
#endif

    return 0;
}

// sfx(n, [channel,] [offset,] [length])
int sfx(lua_State *L)
{
#ifdef ENABLE_AUDIO
    int n = lua_tonumber(L, 1);
    int channel = lua_to_or_default(L, number, 2, -1);
    int offset = lua_to_or_default(L, number, 3, 0);
    int length = lua_to_or_default(L, number, 3, 32);

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
    int celx = lua_tonumber(L, 1);
    int cely = lua_tonumber(L, 2);
    int sx = lua_tonumber(L, 3);
    int sy = lua_tonumber(L, 4);
    int celw = lua_gettop(L) >= 5 ? lua_tonumber(L, 5) : P8_WIDTH / SPRITE_WIDTH;
    int celh = lua_gettop(L) >= 6 ? lua_tonumber(L, 6) : P8_HEIGHT / SPRITE_HEIGHT;
    int layer = lua_gettop(L) >= 7 ? lua_tonumber(L, 7) : 0;

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
    int celx = lua_tonumber(L, 1);
    int cely = lua_tonumber(L, 2);

    lua_pushnumber(L, map_get(celx, cely));

    return 1;
}

// mset(celx, cely, snum)
int mset(lua_State *L)
{
    int celx = lua_tonumber(L, 1);
    int cely = lua_tonumber(L, 2);
    int snum = lua_tonumber(L, 3);

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
    int destaddr = lua_tonumber(L, 1);
    int sourceaddr = lua_tonumber(L, 2);
    int len = lua_tonumber(L, 3);

    if (len < 1 || destaddr == sourceaddr)
        return 0;

    if (sourceaddr < MEMORY_SCREEN || sourceaddr + len > MEMORY_SCREEN + MEMORY_SCREEN_SIZE)
        return 0;

    if (destaddr < MEMORY_SCREEN || destaddr + len > MEMORY_SCREEN + MEMORY_SCREEN_SIZE)
        return 0;

    memcpy(m_memory + destaddr, m_memory + sourceaddr, len);

    return 0;
}

// memset(destaddr, val, len)
int _memset(lua_State *L)
{
    int destaddr = lua_tonumber(L, 1);
    int val = lua_tonumber(L, 2);
    int len = lua_tonumber(L, 3);

    memset(m_memory + destaddr, val, len);

    return 0;
}

// peek(addr)
int peek(lua_State *L)
{
    int addr = lua_tonumber(L, 1);

    lua_pushnumber(L, m_memory[addr]);

    return 1;
}

// peek2(addr)
int peek2(lua_State *L)
{
    int addr = lua_tonumber(L, 1);

    lua_pushnumber(L, (m_memory[addr + 1] << 8) | m_memory[addr]);

    return 1;
}

// peek4(addr)
int peek4(lua_State *L)
{
    int addr = lua_tonumber(L, 1);
    lua_pushnumber(L, (m_memory[addr + 3] << 24) | (m_memory[addr + 2] << 16) | (m_memory[addr + 1] << 8) | m_memory[addr]);

    return 1;
}

// poke(addr, val)
int poke(lua_State *L)
{
    int addr = lua_tonumber(L, 1);
    int val = lua_tonumber(L, 2);

    m_memory[addr] = val;

    return 0;
}

// poke2(addr, val)
int poke2(lua_State *L)
{
    int addr = lua_tonumber(L, 1);
    int val = lua_tonumber(L, 2);

    m_memory[addr] = val;
    m_memory[addr + 1] = val >> 8;

    return 0;
}

// poke4(addr, val)
int poke4(lua_State *L)
{
    int addr = lua_tonumber(L, 1);
    int val = lua_tonumber(L, 2);

    m_memory[addr] = val;
    m_memory[addr + 1] = val >> 8;
    m_memory[addr + 2] = val >> 16;
    m_memory[addr + 3] = val >> 24;

    return 0;
}

// reload(destaddr, sourceaddr, len, [filename])

// ****************************************************************
// *** Math ***
// ****************************************************************

// abs(num)
int _abs(lua_State *L)
{
    if (lua_isnumber(L, 1))
    {
        float num = lua_tonumber(L, 1);
        lua_pushnumber(L, fabsf(num));
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// atan2(dx, dy)
int _atan2(lua_State *L)
{
    assert(lua_isnumber(L, 1));

    float dx = lua_tonumber(L, 1);
    float dy = lua_tonumber(L, 2);
    float value = (float)(atan2f(dx, dy) / TWO_PI - 0.25f);

    if (value < 0.0f)
        value += 1.0f;

    lua_pushnumber(L, value);

    return 1;
}

// band(first, second)
int band(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int first = lua_tonumber(L, 1);
        int second = lua_tonumber(L, 2);

        lua_pushnumber(L, first & second);
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// bnot(num)
int bnot(lua_State *L)
{
    assert(lua_isnumber(L, 1));

    int num = lua_tonumber(L, 1);

    lua_pushnumber(L, ~num);

    return 1;
}

// bor(first, second)
int bor(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int first = lua_tonumber(L, 1);
        int second = lua_tonumber(L, 2);

        lua_pushnumber(L, first | second);
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// bxor(first, second)
int bxor(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int first = lua_tonumber(L, 1);
        int second = lua_tonumber(L, 2);

        lua_pushnumber(L, first ^ second);
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// ceil(num)
int _ceil(lua_State *L)
{
    float num = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 0;
    lua_pushnumber(L, ceilf(num));

    return 1;
}

// cos(angle)
int _cos(lua_State *L)
{
    if (lua_isnumber(L, 1))
    {
        float angle = lua_tonumber(L, 1);
        lua_pushnumber(L, cosf((1.0f - angle) * TWO_PI));
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// flr(num)
int flr(lua_State *L)
{
    float num = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 0;
    lua_pushnumber(L, floorf(num));

    return 1;
}

// lshr(num, bits)
int lshr(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int num = lua_tonumber(L, 1);
        int bits = lua_tonumber(L, 2);

        lua_pushnumber(L, (num >> bits) ^ (num & 0xFFFFFF));
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// max(first, [second])
int max(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        float first = lua_tonumber(L, 1);
        float second = lua_tonumber(L, 2);

        lua_pushnumber(L, fmaxf(first, second));
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// mid(first, second, third)
int mid(lua_State *L)
{
    float first = lua_tonumber(L, 1);
    float second = lua_tonumber(L, 2);
    float third = lua_gettop(L) >= 3 ? lua_tonumber(L, 3) : 0;

    if ((first < second && second < third) || (third < second && second < first))
        lua_pushnumber(L, second);
    else if ((second < first && first < third) || (third < first && first < second))
        lua_pushnumber(L, first);
    else
        lua_pushnumber(L, third);

    return 1;
}

// min(first, [second])
int min(lua_State *L)
{
    float first = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 0;
    float second = 0;

    if (lua_gettop(L) == 2 && lua_isnumber(L, 2))
        second = lua_tonumber(L, 2);

    lua_pushnumber(L, fminf(first, second));

    return 1;
}

// rnd(max)
int rnd(lua_State *L)
{
    float max = lua_gettop(L) >= 1 ? lua_tonumber(L, 1) : 1.0f;

    lua_pushnumber(L, ((float)rand() / RAND_MAX) * max);

    return 1;
}

// rotl(num, bits)
int rotl(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int num = lua_tonumber(L, 1);
        int bits = lua_tonumber(L, 2);

        lua_pushnumber(L, (num << bits) | (num >> (32 - bits)));
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// rotr(num, bits)
int rotr(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int num = lua_tonumber(L, 1);
        int bits = lua_tonumber(L, 2);

        lua_pushnumber(L, (num >> bits) | (num << (32 - bits)));
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// sgn([number])
int sgn(lua_State *L)
{
    assert(lua_isnumber(L, 1));

    float number = lua_tonumber(L, 1);
    lua_pushnumber(L, number > 0 ? 1.0f : -1.0f);

    return 1;
}

// shl(num, bits)
int shl(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int num = lua_tonumber(L, 1);
        int bits = lua_tonumber(L, 2);

        lua_pushnumber(L, num << bits);
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// shr(num, bits)
int shr(lua_State *L)
{
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int num = lua_tonumber(L, 1);
        int bits = lua_tonumber(L, 2);

        lua_pushnumber(L, num >> bits);
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// sin(angle)
int _sin(lua_State *L)
{
    if (lua_isnumber(L, 1))
    {
        float angle = lua_tonumber(L, 1);
        lua_pushnumber(L, sinf((1.0f - angle) * TWO_PI));
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

// sqrt(num)
int _sqrt(lua_State *L)
{
    assert(lua_isnumber(L, 1));

    float num = lua_tonumber(L, 1);
    lua_pushnumber(L, sqrtf(num));

    return 1;
}

// srand(val)

// ****************************************************************
// *** Cartridge data ***
// ****************************************************************

// cartdata(id)
int cartdata(lua_State *L)
{
    return 0;
}

// dget(index)
int dget(lua_State *L)
{
    int index = lua_tonumber(L, 1);

    lua_pushnumber(L, m_memory[MEMORY_CARTDATA + index]);

    return 1;
}

// dset(index, value)
int dset(lua_State *L)
{
    int index = lua_tonumber(L, 1);
    int value = lua_tonumber(L, 2);

    m_memory[MEMORY_CARTDATA + index] = value;

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

// chr(ord)
int chr(lua_State *L)
{
    int ord = lua_tonumber(L, 1);

    lua_pushnumber(L, ord);

    return 1;
}

// setmetatable(tbl, metatbl)
// getmetatable(tbl)
// type(v)
// sub(str, from, [to])
int sub(lua_State *L)
{
    const char *str = lua_tostring(L, 1);
    int start = lua_tonumber(L, 2);
    int end = lua_to_or_default(L, number, 3, -1);

    if (end == -1)
    {
        strncpy(m_str_buffer, str + start, strlen(str) - start);

        lua_pushstring(L, m_str_buffer);

        return 1;
    }

    strncpy(m_str_buffer, str + start, end - start);

    lua_pushstring(L, m_str_buffer);

    return 1;
}

// tonum(str)
// tostr(val, [usehex])
int tostr(lua_State *L)
{
    switch (lua_type(L, 1))
    {
    case LUA_TBOOLEAN:
        lua_pushstring(L, lua_toboolean(L, 1) ? "true" : "false");
        break;
    case LUA_TNUMBER:
    {
        bool usehex = lua_gettop(L) >= 2 ? lua_toboolean(L, 2) : false;
        if (usehex)
            sprintf(m_str_buffer, "%x", (int)lua_tonumber(L, 1));
        else
            sprintf(m_str_buffer, "%4.4f", lua_tonumber(L, 1));
        lua_pushstring(L, m_str_buffer);
        break;
    }
    case LUA_TSTRING:
        lua_pushstring(L, lua_tostring(L, 1));
        break;
    default:
        lua_pushstring(L, "");
        break;
    }

    return 1;
}

// ****************************************************************
// *** Time ***
// ****************************************************************

// time()
int _time(lua_State *L)
{
    lua_pushnumber(L, m_time);

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
    int n = lua_tonumber(L, -1);

    switch (n)
    {
    case STAT_FRAMERATE:
        lua_pushnumber(L, m_fps);
        break;
    default:
        lua_pushnumber(L, 0);
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
    int fps = lua_tonumber(L, 1);
    m_fps = MAX(0, fps);

    return 0;
}

int get_mouse_x(lua_State *L)
{
    lua_pushnumber(L, m_mouse_x);

    return 1;
}

int get_mouse_y(lua_State *L)
{
    lua_pushnumber(L, m_mouse_y);

    return 1;
}

int note_to_hz(lua_State *L)
{
    int note = lua_tonumber(L, 1);

    lua_pushnumber(L, 440 * 2 ^ ((note - 33) / 12));

    return 1;
}

void clear_screen()
{
    int color = lua_gettop(L) == 1 ? lua_tonumber(L, -1) : 0;

    reset_color();
    // memset(MEMORY_SCREEN, color, 0x2000);

    for (int y = 0; y < P8_HEIGHT; y++)
        for (int x = 0; x < P8_WIDTH; x++)
            gfx_set(x, y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE, color);

    clip_set(0, 0, P8_WIDTH, P8_HEIGHT);
    cursor_set(0, 0, -1);
}

int dash_direction_x(int facing)
{
    return 0;
}

int dash_direction_y(int facing)
{
    return 0;
}

void draw_circ_segment(int xc, int yc, int x, int y, int col)
{
    pixel_set(xc + x, yc + y, col);
    pixel_set(xc - x, yc + y, col);
    pixel_set(xc + x, yc - y, col);
    pixel_set(xc - x, yc - y, col);
    pixel_set(xc + y, yc + x, col);
    pixel_set(xc - y, yc + x, col);
    pixel_set(xc + y, yc - x, col);
    pixel_set(xc - y, yc - x, col);
}

void draw_circ(int xc, int yc, int r, int col)
{
    int x = 0, y = abs(r);
    int d = 3 - 2 * abs(r);

    draw_circ_segment(xc, yc, x, y, col);

    while (y >= x)
    {
        x++;

        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;

        draw_circ_segment(xc, yc, x, y, col);
    }
}

void draw_line(int x0, int y0, int x1, int y1, int col)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
        pixel_set(x0, y0, col);

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
    }
}

void draw_circfill_segment(int xc, int yc, int x, int y, int col)
{
    draw_line(xc + x, yc + y, xc - x, yc + y, col);
    draw_line(xc + x, yc - y, xc - x, yc - y, col);
    draw_line(xc + y, yc + x, xc - y, yc + x, col);
    draw_line(xc + y, yc - x, xc - y, yc - x, col);
}

void draw_circfill(int xc, int yc, int r, int col)
{
    int x = 0, y = abs(r);
    int d = 3 - 2 * abs(r);

    draw_circfill_segment(xc, yc, x, y, col);

    while (y >= x)
    {
        x++;

        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;

        draw_circfill_segment(xc, yc, x, y, col);
    }
}

void pixel_set(int x, int y, int c)
{
    int cx, cy;
    camera_get(&cx, &cy);
    x -= cx;
    y -= cy;
    int x0 = m_memory[MEMORY_CLIPRECT];
    int y0 = m_memory[MEMORY_CLIPRECT + 1];
    int x1 = m_memory[MEMORY_CLIPRECT + 2];
    int y1 = m_memory[MEMORY_CLIPRECT + 3];

    if (x >= x0 && x < x1 && y >= y0 && y < y1)
    {
        int col = color_get(PALTYPE_DRAW, c);
        gfx_set(x, y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE, col);
    }
}

void draw_scaled_sprite(int sx, int sy, int sw, int sh, int dx, int dy, float scale_x, float scale_y, bool flip_x, bool flip_y)
{
    for (int y = 0; y < sh * scale_y; y++)
    {
        for (int x = 0; x < sw * scale_x; x++)
        {
            int src_x = flip_x ? sx + (sw - x / scale_x) : sx + x / scale_x;
            int src_y = flip_y ? sy + (sh - y / scale_y) : sy + y / scale_y;
            uint8_t index = gfx_get(src_x, src_y, MEMORY_SPRITES, MEMORY_SPRITES_SIZE);
            uint8_t background = gfx_get(dx + x, dy + y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE);
            uint8_t color = color_get(PALTYPE_DRAW, (int)index);

            if ((color & 0x10) == 0)
                pixel_set(dx + x, dy + y, color == 0 ? background : color);
        }
    }
}

void draw_sprite(int n, int left, int top, bool flip_x, bool flip_y)
{
    int sx = n % 16 * SPRITE_WIDTH;
    int sy = n / 16 * SPRITE_HEIGHT;

    for (int y = 0; y < SPRITE_HEIGHT; y++)
    {
        for (int x = 0; x < SPRITE_WIDTH; x++)
        {
            int fx = flip_x ? (SPRITE_WIDTH - x - 1) : x;
            int fy = flip_y ? (SPRITE_HEIGHT - y - 1) : y;
            uint8_t background = gfx_get(left + fx, top + fy, MEMORY_SCREEN, MEMORY_SCREEN_SIZE);
            uint8_t index = gfx_get(sx + x, sy + y, MEMORY_SPRITES, MEMORY_SPRITES_SIZE);
            uint8_t color = color_get(PALTYPE_DRAW, index);

            if ((color & 0x10) == 0)
                pixel_set(left + fx, top + fy, (color == 0 ? background : color));
        }
    }
}

void draw_sprites(int n, int x, int y, float w, float h, bool flip_x, bool flip_y)
{
    for (int sy = 0; sy < h; sy++)
    {
        for (int sx = 0; sx < w; sx++)
        {
            int index = (n == -1.0f ? 0 : n) + sx + sy * 16;
            int left = x + sx * 8;
            int top = y + sy * 8;
            draw_sprite(index, left, top, flip_x, flip_y);
        }
    }
}

void draw_char(int n, int left, int top, int col)
{
    int sx = n % 16;
    int sy = n / 16 * 5;

    for (int y = 0; y < 8; y++)
    {
        uint8_t x_byte = y >= 5 ? 0 : font_map[sx + (sy + y) * 16];

        for (int x = 0; x < 8; x++)
        {
            uint8_t background = gfx_get(left + x, top + y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE);
            uint8_t x_bit = (x_byte >> (7 - x)) & 0x1;

            pixel_set(left + x, top + y, x_bit ? (col == -1 ? (int)pencolor_get() : col) : background);
        }
    }
}

int get_p8_symbol(const char *str, uint8_t *symbol_length)
{
    int p8_symbols_len = sizeof(p8_symbols) / sizeof(p8_symbol_t);
    int str_len = strlen(str);

    for (int i = 0; i < p8_symbols_len; i++)
    {
        const uint8_t *encoding = p8_symbols[i].encoding;
        *symbol_length = p8_symbols[i].length;

        if (*symbol_length > str_len)
            continue;

        if (memcmp(str, encoding, *symbol_length) == 0)
            return p8_symbols[i].index;
    }

    return -1;
}

int draw_text(const char *str, int x, int y, int col)
{
    int left = 0;
    int str_len = strlen(str);

    for (int i = 0; i < str_len; i++)
    {
        uint8_t character = str[i];
        int index = -1;

        if (character >= 0x20 && character < 0x7F)
        {
            index = character;
        }
        else
        {
            uint8_t symbol_length = 0;
            index = get_p8_symbol(&str[i], &symbol_length);

            if (index != -1)
                i += symbol_length - 1;
        }

        if (index >= 0)
            draw_char(index, (int)x + left, (int)y, col);

        left += GLYPH_WIDTH;
    }

    return left;
}

uint8_t gfx_get(int x, int y, int location, int size)
{
    int offset = location + (x >> 1) + y * 64;

    return IS_EVEN(x) ? m_memory[offset] & 0xF : m_memory[offset] >> 4;
}

void gfx_set(int x, int y, int location, int size, int col)
{
    int offset = location + (x >> 1) + y * 64;

    uint8_t color = (col == -1 ? pencolor_get() : color_get(PALTYPE_DRAW, col));

    m_memory[offset] = IS_EVEN(x) ? (m_memory[offset] & 0xF0) | (color & 0xF) : (color << 4) | (m_memory[offset] & 0xF);
}

void camera_get(int *x, int *y)
{
    *x = (m_memory[MEMORY_CAMERA + 1] << 8) | m_memory[MEMORY_CAMERA];
    *y = (m_memory[MEMORY_CAMERA + 3] << 8) | m_memory[MEMORY_CAMERA + 2];
}

void camera_set(int x, int y)
{
    m_memory[MEMORY_CAMERA] = x & 0xF;
    m_memory[MEMORY_CAMERA + 1] = x >> 8;
    m_memory[MEMORY_CAMERA + 2] = y & 0xF;
    m_memory[MEMORY_CAMERA + 3] = y >> 8;
}

uint8_t pencolor_get()
{
    return m_memory[MEMORY_PENCOLOR];
}

void pencolor_set(uint8_t col)
{
    m_memory[MEMORY_PENCOLOR] = col;
}

uint8_t color_get(int type, int index)
{
    return m_memory[MEMORY_PALETTES + type * 16 + index];
}

void color_set(int type, int index, int col)
{
    m_memory[MEMORY_PALETTES + type * 16 + index] = col;
}

void clip_set(int x, int y, int w, int h)
{
    m_memory[MEMORY_CLIPRECT] = x;
    m_memory[MEMORY_CLIPRECT + 1] = y;
    m_memory[MEMORY_CLIPRECT + 2] = x + w;
    m_memory[MEMORY_CLIPRECT + 3] = y + h;
}

void cursor_set(int x, int y, int col)
{
    m_memory[MEMORY_CURSOR] = x;
    m_memory[MEMORY_CURSOR + 1] = y;

    if (col != -1)
        pencolor_set(col);
}

void cursor_get(int *x, int *y)
{
    *x = (int)m_memory[MEMORY_CURSOR];
    *y = (int)m_memory[MEMORY_CURSOR + 1];
}

uint8_t map_get(int celx, int cely)
{
    if (cely < 32)
        return m_memory[MEMORY_MAP + celx + cely * P8_WIDTH];
    else
        return m_memory[MEMORY_SPRITES_MAP + celx + (cely - 32) * P8_WIDTH];
}

void map_set(int celx, int cely, int snum)
{
    if (cely < 32)
        m_memory[MEMORY_MAP + celx + cely * P8_WIDTH] = snum;
    else
        m_memory[MEMORY_SPRITES_MAP + celx + (cely - 32) * P8_WIDTH] = snum;
}

void reset_color()
{
    for (int i = 0; i < 16; i++)
    {
        color_set(PALTYPE_DRAW, i, i == 0 ? i | 0x10 : i);
        color_set(PALTYPE_SCREEN, i, i == 0 ? i | 0x10 : i);
    }
}

bool is_button_set(int index, int button, bool prev_buttons)
{
    uint8_t mask = prev_buttons ? m_prev_buttons[index] : m_buttons[index];

    if (mask == 0)
        return false;

    switch (button)
    {
    case 0:
        return (mask & BUTTON_LEFT);
    case 1:
        return (mask & BUTTON_RIGHT);
    case 2:
        return (mask & BUTTON_UP);
    case 3:
        return (mask & BUTTON_DOWN);
    case 4:
        return (mask & BUTTON_ACTION1);
    case 5:
        return (mask & BUTTON_ACTION2);
    }

    return false;
}

void update_buttons(int index, int button, bool state)
{
    uint8_t mask = m_buttons[index];

    switch (button)
    {
    case 0:
        mask = state ? mask | BUTTON_LEFT : mask & ~BUTTON_LEFT;
        break;
    case 1:
        mask = state ? mask | BUTTON_RIGHT : mask & ~BUTTON_RIGHT;
        break;
    case 2:
        mask = state ? mask | BUTTON_UP : mask & ~BUTTON_UP;
        break;
    case 3:
        mask = state ? mask | BUTTON_DOWN : mask & ~BUTTON_DOWN;
        break;
    case 4:
        mask = state ? mask | BUTTON_ACTION1 : mask & ~BUTTON_ACTION1;
        break;
    case 5:
        mask = state ? mask | BUTTON_ACTION2 : mask & ~BUTTON_ACTION2;
        break;
    default:
        break;
    }

    m_prev_buttons[index] = m_buttons[index];
    m_buttons[index] = mask;
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
    lua_register(L, "pal", pal);
    lua_register(L, "palt", palt);
    lua_register(L, "pget", pget);
    lua_register(L, "print", print);
    lua_register(L, "pset", pset);
    lua_register(L, "rect", rect);
    lua_register(L, "rectfill", rectfill);
    lua_register(L, "sget", sget);
    lua_register(L, "spr", spr);
    lua_register(L, "sset", sset);
    lua_register(L, "sspr", sspr);
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
    // lua_register(L, "reload", reload);
    // ****************************************************************
    // *** Math ***
    // ****************************************************************
    lua_register(L, "abs", _abs);
    lua_register(L, "atan2", _atan2);
    lua_register(L, "band", band);
    lua_register(L, "bnot", bnot);
    lua_register(L, "bor", bor);
    lua_register(L, "bxor", bxor);
    lua_register(L, "ceil", _ceil);
    lua_register(L, "cos", _cos);
    lua_register(L, "flr", flr);
    lua_register(L, "lshr", lshr);
    lua_register(L, "max", max);
    lua_register(L, "mid", mid);
    lua_register(L, "min", min);
    lua_register(L, "rnd", rnd);
    lua_register(L, "rotl", rotl);
    lua_register(L, "rotr", rotr);
    lua_register(L, "sgn", sgn);
    lua_register(L, "shl", shl);
    lua_register(L, "shr", shr);
    lua_register(L, "sin", _sin);
    lua_register(L, "sqrt", _sqrt);
    // lua_register(L, "srand", srand);
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
    lua_register(L, "chr", chr);
    // lua_register(L, "setmetatable", setmetatable);
    // lua_register(L, "getmetatable", getmetatable);
    // lua_register(L, "type", type);
    lua_register(L, "sub", sub);
    // lua_register(L, "tonum", tonum);
    lua_register(L, "tostr", tostr);
    // ****************************************************************
    // *** Time ***
    // ****************************************************************
    lua_register(L, "time", _time);
    // ****************************************************************
    // *** System ***
    // ****************************************************************
    lua_register(L, "menuitem", menuitem);
    // lua_register(L, "extcmd", extcmd);
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
}

void lua_shutdown_api()
{
    if (L)
        lua_close(L);
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

void lua_init()
{
    if (m_lua_init)
        lua_call_function("_init", 0);
}
