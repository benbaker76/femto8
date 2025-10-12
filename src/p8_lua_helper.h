/*
 * p8_lua_helper.h
 *
 *  Created on: Dec 26, 2023
 *      Author: bbaker
 */

#ifndef P8_LUA_HELPER_H
#define P8_LUA_HELPER_H

#include <stdbool.h>
#include "p8_emu.h"
#include "p8_symbols.h"
#include "pico_font.h"

static inline void clear_screen(int color);
static inline void draw_circ(int x, int y, int r, int col);
static inline void draw_circfill(int x, int y, int r, int col);
static inline void draw_sprite(int n, int left, int top, bool flip_x, bool flip_y);
static inline void draw_scaled_sprite(int sx, int sy, int sw, int sh, int dx, int dy, float scale_x, float scale_y, bool flip_x, bool flip_y);
static inline void draw_sprites(int n, int x, int y, float w, float h, bool flip_x, bool flip_y);
static inline void draw_line(int x0, int y0, int x1, int y1, int col);
static inline void draw_char(int n, int left, int top, int col);
static inline uint8_t gfx_addr_get(int x, int y, uint8_t *memory, int location, int size);
static inline uint8_t gfx_get(int x, int y, int location, int size);
static inline void gfx_set(int x, int y, int location, int size, int col);
static inline void camera_get(int *x, int *y);
static inline void camera_set(int x, int y);
static inline uint8_t pencolor_get();
static inline void pencolor_set(uint8_t col);
static inline uint8_t color_get(int type, int index);
static inline void color_set(int type, int index, int col);
static inline void clip_set(int x, int y, int w, int h);
static inline void cursor_get(int *x, int *y);
static inline void cursor_set(int x, int y, int col);
static inline void pixel_set(int x, int y, int c);
static inline int draw_text(const char *str, int x, int y, int col);
static inline bool is_button_set(int index, int button, bool prev_buttons);
static inline void update_buttons(int index, int button, bool state);
static inline void map_set(int celx, int cely, int snum);
static inline uint8_t map_get(int celx, int cely);
static inline void reset_color();

static inline void clear_screen(int color)
{
    reset_color();
    // memset(MEMORY_SCREEN, color, 0x2000);

    for (int y = 0; y < P8_HEIGHT; y++)
        for (int x = 0; x < P8_WIDTH; x++)
            gfx_set(x, y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE, color);

    clip_set(0, 0, P8_WIDTH - 1, P8_HEIGHT - 1);
    cursor_set(0, 0, -1);
}

static inline int dash_direction_x(int facing)
{
    return 0;
}

static inline int dash_direction_y(int facing)
{
    return 0;
}

static inline void draw_circ_segment(int xc, int yc, int x, int y, int col)
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

static inline void draw_circ(int xc, int yc, int r, int col)
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

static inline void draw_line(int x0, int y0, int x1, int y1, int col)
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

static inline void draw_circfill_segment(int xc, int yc, int x, int y, int col)
{
    draw_line(xc + x, yc + y, xc - x, yc + y, col);
    draw_line(xc + x, yc - y, xc - x, yc - y, col);
    draw_line(xc + y, yc + x, xc - y, yc + x, col);
    draw_line(xc + y, yc - x, xc - y, yc - x, col);
}

static inline void draw_circfill(int xc, int yc, int r, int col)
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

static inline void pixel_set(int x, int y, int c)
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

static inline void draw_scaled_sprite(int sx, int sy, int sw, int sh, int dx, int dy, float scale_x, float scale_y, bool flip_x, bool flip_y)
{
    for (int y = 0; y < sh * scale_y; y++)
    {
        for (int x = 0; x < sw * scale_x; x++)
        {
            int src_x = flip_x ? sx + (sw - x / scale_x) : sx + x / scale_x;
            int src_y = flip_y ? sy + (sh - y / scale_y) : sy + y / scale_y;
            uint8_t background = gfx_get(dx + x, dy + y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE);
            uint8_t index = gfx_get(src_x, src_y, MEMORY_SPRITES, MEMORY_SPRITES_SIZE);
            uint8_t color = color_get(PALTYPE_DRAW, (int)index);

            if ((color & 0x10) == 0)
                pixel_set(dx + x, dy + y, color == 0 ? background : color);
        }
    }
}

static inline void draw_sprite(int n, int left, int top, bool flip_x, bool flip_y)
{
    int sx = (n & 0xF) * SPRITE_WIDTH;
    int sy = (n >> 4) * SPRITE_HEIGHT;

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

static inline void draw_sprites(int n, int x, int y, float w, float h, bool flip_x, bool flip_y)
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

static inline void draw_char(int n, int left, int top, int col)
{
    int sx = n % 16 * 8;
    int sy = n / 16 * 8;

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            uint8_t background = gfx_get(left + x, top + y, MEMORY_SCREEN, MEMORY_SCREEN_SIZE);
            uint8_t index = gfx_addr_get(sx + x, sy + y, (uint8_t *)font_map, 0, sizeof(font_map));
            uint8_t color = color_get(PALTYPE_DRAW, index);

            pixel_set(left + x, top + y, color == 7 ? (col == -1 ? (int)pencolor_get() : col) : background);
        }
    }
}

static inline int get_p8_symbol(const char *str, int str_len, uint8_t *symbol_length)
{
    int p8_symbols_len = sizeof(p8_symbols) / sizeof(p8_symbol_t);

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

static inline int draw_text(const char *str, int x, int y, int col)
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
            index = get_p8_symbol(&str[i], str_len - i, &symbol_length);

            if (index != -1)
                i += symbol_length - 1;
        }

        if (index >= 0)
            draw_char(index, (int)x + left, (int)y, col);

        left += GLYPH_WIDTH;
    }

    return left;
}

static inline uint8_t gfx_addr_get(int x, int y, uint8_t *memory, int location, int size)
{
    int offset = location + (x >> 1) + y * 64;

    return IS_EVEN(x) ? memory[offset] & 0xF : memory[offset] >> 4;
}

static inline uint8_t gfx_get(int x, int y, int location, int size)
{
    return gfx_addr_get(x, y, m_memory, location, size);
}

static inline void gfx_set(int x, int y, int location, int size, int col)
{
    int offset = location + (x >> 1) + y * 64;

    uint8_t color = (col == -1 ? pencolor_get() : color_get(PALTYPE_DRAW, col));

    m_memory[offset] = IS_EVEN(x) ? (m_memory[offset] & 0xF0) | (color & 0xF) : (color << 4) | (m_memory[offset] & 0xF);
}

static inline void camera_get(int *x, int *y)
{
    *x = (m_memory[MEMORY_CAMERA + 1] << 8) | m_memory[MEMORY_CAMERA];
    *y = (m_memory[MEMORY_CAMERA + 3] << 8) | m_memory[MEMORY_CAMERA + 2];
}

static inline void camera_set(int x, int y)
{
    m_memory[MEMORY_CAMERA] = x & 0xF;
    m_memory[MEMORY_CAMERA + 1] = x >> 8;
    m_memory[MEMORY_CAMERA + 2] = y & 0xF;
    m_memory[MEMORY_CAMERA + 3] = y >> 8;
}

static inline uint8_t pencolor_get()
{
    return m_memory[MEMORY_PENCOLOR];
}

static inline void pencolor_set(uint8_t col)
{
    m_memory[MEMORY_PENCOLOR] = col;
}

static inline uint8_t color_get(int type, int index)
{
    return m_memory[MEMORY_PALETTES + type * 16 + index];
}

static inline void color_set(int type, int index, int col)
{
    m_memory[MEMORY_PALETTES + type * 16 + index] = col;
}

static inline void clip_set(int x, int y, int w, int h)
{
    m_memory[MEMORY_CLIPRECT] = x;
    m_memory[MEMORY_CLIPRECT + 1] = y;
    m_memory[MEMORY_CLIPRECT + 2] = x + w;
    m_memory[MEMORY_CLIPRECT + 3] = y + h;
}

static inline void cursor_set(int x, int y, int col)
{
    m_memory[MEMORY_CURSOR] = x;
    m_memory[MEMORY_CURSOR + 1] = y;

    if (col != -1)
        pencolor_set(col);
}

static inline void cursor_get(int *x, int *y)
{
    *x = (int)m_memory[MEMORY_CURSOR];
    *y = (int)m_memory[MEMORY_CURSOR + 1];
}

static inline uint8_t map_get(int celx, int cely)
{
    if (cely < 32)
        return m_memory[MEMORY_MAP + celx + cely * P8_WIDTH];
    else
        return m_memory[MEMORY_SPRITES_MAP + celx + (cely - 32) * P8_WIDTH];
}

static inline void map_set(int celx, int cely, int snum)
{
    if (cely < 32)
        m_memory[MEMORY_MAP + celx + cely * P8_WIDTH] = snum;
    else
        m_memory[MEMORY_SPRITES_MAP + celx + (cely - 32) * P8_WIDTH] = snum;
}

static inline void reset_color()
{
    for (int i = 0; i < 16; i++)
    {
        color_set(PALTYPE_DRAW, i, i == 0 ? i | 0x10 : i);
        color_set(PALTYPE_SCREEN, i, i == 0 ? i | 0x10 : i);
    }
}

static inline bool is_button_set(int index, int button, bool btnp)
{
    uint8_t mask = (btnp?m_buttonsp:m_buttons)[index];

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

static inline void update_buttons(int index, int button, bool state)
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

    m_buttons[index] = mask;
}

#endif
