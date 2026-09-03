/*
 * p8_print_helper.h
 *
 *  Print and P8SCII support
 */

#ifndef P8_PRINT_HELPER_H
#define P8_PRINT_HELPER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "p8_emu.h"
#include "p8_symbols.h"
#include "pico_font.h"
#include "p8_audio.h"
#include "p8_lua_helper.h"

static inline int hexy(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    else
        return -1;
}

typedef struct {
    bool wide;
    bool tall;
    bool stripey;
    bool invert;
    bool border;
    bool solid_bg;
    int char_w;
    int char_h;
    int char_w2;
    bool outline_enabled;
    int outline_colour;
    int outline_mask;
    bool outline_skip_interior;
    bool use_custom_font;
    int delay_frames;
} print_state_t;

static inline int get_custom_font_char_width(int char_index)
{
    int base_width = (char_index < 128) ? m_memory[MEMORY_FONT] : m_memory[MEMORY_FONT + 1];

    if (!(m_memory[MEMORY_FONT + 5] & 0x1) || char_index < 16) {
        return base_width;
    }

    int nibble_index = char_index - 16;
    int byte_offset = 8 + nibble_index / 2;
    uint8_t byte_val = m_memory[MEMORY_FONT + byte_offset];

    int nibble = (nibble_index & 1) ? (byte_val >> 4) : (byte_val & 0xF);

    int adjust = (nibble & 0x7);
    if (adjust >= 4) adjust -= 8;

    return base_width + adjust;
}

static inline bool get_custom_font_y_offset(int char_index)
{
    if (!(m_memory[MEMORY_FONT + 5] & 0x1) || char_index < 16) {
        return false;
    }

    int nibble_index = char_index - 16;
    int byte_offset = 8 + nibble_index / 2;
    uint8_t byte_val = m_memory[MEMORY_FONT + byte_offset];

    int nibble = (nibble_index & 1) ? (byte_val >> 4) : (byte_val & 0xF);
    return (nibble & 0x8) != 0;
}

static inline bool get_font_pixel(int char_index, int x, int y, bool use_custom_font)
{
    if (use_custom_font) {
        if (char_index < 16) {
            int sx = (char_index % 16) * 8;
            int sy = (char_index / 16) * 8;
            uint8_t index = gfx_addr_get(sx + x, sy + y, (uint8_t *)font_map, 0, sizeof(font_map));
            return (index == 7);
        }
        int offset = MEMORY_FONT + 128 + (char_index - 16) * 8 + y;
        if (offset < MEMORY_FONT + MEMORY_FONT_SIZE) {
            uint8_t row = m_memory[offset];
            return (row & (1 << x)) != 0;
        }
        return false;
    } else {
        int sx = (char_index % 16) * 8;
        int sy = (char_index / 16) * 8;
        uint8_t index = gfx_addr_get(sx + x, sy + y, (uint8_t *)font_map, 0, sizeof(font_map));
        return (index == 7);
    }
}

static inline void draw_char_styled(int n, int left, int top, int fg, int bg, print_state_t *state)
{
    int x_offset = 0;
    int y_offset = 0;

    if (state->use_custom_font) {
        x_offset += m_memory[MEMORY_FONT + 3];
        y_offset += m_memory[MEMORY_FONT + 4];

        if (get_custom_font_y_offset(n)) {
            y_offset -= 1;
        }
    }

    int scale_x = state->wide ? 2 : 1;
    int scale_y = state->tall ? 2 : 1;

    int char_w = state->use_custom_font ? get_custom_font_char_width(n) : (n >= 0x80 ? state->char_w2 : state->char_w);
    int render_width = char_w;
    int render_height = state->char_h;
    if (render_width > 8) render_width = 8;
    if (render_height > 8) render_height = 8;

    int outline_dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int outline_dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int y = 0; y < state->char_h; y++) {
        for (int x = 0; x < char_w; x++) {
            bool is_fg = (x < render_width && y < render_height) ? get_font_pixel(n, x, y, state->use_custom_font) : false;

            if (state->invert)
                is_fg = !is_fg;

            int colour = is_fg ? fg : bg;
            if (colour == -1 && !is_fg) continue;

            if (state->outline_enabled && is_fg) {
                for (int nb = 0; nb < 8; nb++) {
                    if (state->outline_mask & (1 << nb)) {
                        int nx = x + outline_dx[nb];
                        int ny = y + outline_dy[nb];

                        bool neighbour_is_fg = false;
                        if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                            neighbour_is_fg = get_font_pixel(n, nx, ny, state->use_custom_font);
                            if (state->invert)
                                neighbour_is_fg = !neighbour_is_fg;
                        }

                        if (!neighbour_is_fg) {
                            for (int dy = 0; dy < scale_y; dy++) {
                                for (int dx = 0; dx < scale_x; dx++) {
                                    if (state->stripey && !((dx + dy) & 1))
                                        continue;
                                    int px = left + x_offset + x * scale_x + dx + outline_dx[nb];
                                    int py = top + y_offset + y * scale_y + dy + outline_dy[nb];
                                    pixel_set(px, py, state->outline_colour, 0, DRAWTYPE_DEFAULT);
                                }
                            }
                        }
                    }
                }
            }

            if (!state->outline_skip_interior || !is_fg) {
                for (int dy = 0; dy < scale_y; dy++) {
                    for (int dx = 0; dx < scale_x; dx++) {
                        if (state->stripey && !((dx + dy) & 1))
                            continue;

                        int px = left + x_offset + x * scale_x + dx;
                        int py = top + y_offset + y * scale_y + dy;
                        pixel_set(px, py, colour, 0, DRAWTYPE_DEFAULT);
                    }
                }
            }
        }
    }
}

static inline void draw_text(const char *str, unsigned str_len, int x, int y, int col, int left_margin, bool no_scroll, int *out_left, int *out_top, int *out_right, int *out_bottom)
{
    int right = x;
    int bottom = y;
    int repeat = 1;
    int bg = 0;
    int fg = col;
    int tab_width = 16;
    int wrap_boundary = 128;
    int last_advance = GLYPH_WIDTH;
    int home_x = x;
    int home_y = y;

    uint8_t text_attrs = m_memory[MEMORY_TEXT_ATTRS];
    bool use_defaults = (text_attrs & 0x1) != 0;

    print_state_t state = {
        .wide = use_defaults && (text_attrs & 0x4),
        .tall = use_defaults && (text_attrs & 0x8),
        .stripey = use_defaults && (text_attrs & 0x40),
        .invert = use_defaults && (text_attrs & 0x20),
        .border = use_defaults ? ((text_attrs & 0x2) != 0) : true,
        .solid_bg = use_defaults && (text_attrs & 0x10),
        .char_w = GLYPH_WIDTH,
        .char_h = GLYPH_HEIGHT,
        .char_w2 = GLYPH_WIDTH * 2,
        .outline_enabled = false,
        .outline_colour = 0,
        .outline_mask = 0,
        .outline_skip_interior = false,
        .use_custom_font = use_defaults && (text_attrs & 0x80),
        .delay_frames = 0
    };

    if (state.use_custom_font) {
        int font_height = m_memory[MEMORY_FONT + 2];
        if (font_height > 0) state.char_h = font_height;
    }

    uint8_t char_size = m_memory[MEMORY_TEXT_CHAR_SIZE];
    uint8_t char_size2 = m_memory[MEMORY_TEXT_CHAR_SIZE2];
    int cw = char_size & 0xf;
    int ch = (char_size >> 4) & 0xf;
    int cw2 = char_size2 & 0xf;
    int tw = (char_size2 >> 4) & 0xf;
    if (cw > 0)
        state.char_w = cw;
    if (cw2 > 0)
        state.char_w2 = cw2;
    if (ch > 0)
        state.char_h = ch;
    if (tw > 0)
        tab_width = tw;

    bool add_newline = (m_memory[MEMORY_MISCFLAGS] & 0x4) == 0;
    bool wrap_enabled = (m_memory[MEMORY_MISCFLAGS] & 0x80) != 0;
    bool memory_no_scroll = (m_memory[MEMORY_MISCFLAGS] & 0x40) != 0;
    bool do_scroll = !memory_no_scroll && !no_scroll;

    for (unsigned i = 0; i < str_len; i++)
    {
        uint8_t character = str[i];
        int index = -1;
        int old_repeat = repeat;
        repeat = 1;

        for (int j = 0; j < old_repeat; j++)
        {
            if (character < 16)
            {
                switch (character) {
                case 0: // "\0"
                    add_newline = false;
                    str_len = 0;
                    break;
                case 1: // "\*"
                    if (i + 1 < str_len) {
                        int r = hexy(str[++i]);
                        if (r != -1)
                            repeat = r;
                    }
                    break;
                case 2: // "\#"
                    if (i + 1 < str_len) {
                        int b = hexy(str[++i]);
                        if (b != -1) {
                            bg = b;
                            state.solid_bg = true;
                        }
                    }
                    break;
                case 3: // "\-"
                    if (i + 1 < str_len) {
                        int offset = hexy(str[++i]);
                        if (offset != -1)
                            x += offset - 16;
                    }
                    break;
                case 4: // "\|"
                    if (i + 1 < str_len) {
                        int offset = hexy(str[++i]);
                        if (offset != -1)
                            y += offset - 16;
                    }
                    break;
                case 5: // "\+"
                    if (i + 2 < str_len) {
                        int x_offset = hexy(str[++i]);
                        int y_offset = hexy(str[++i]);
                        if (x_offset != -1 && y_offset != -1) {
                            x += x_offset - 16;
                            y += y_offset - 16;
                        }
                    }
                    break;
                case 6: // "\^"
                    if (i + 1 < str_len) {
                        uint8_t cmd = str[++i];
                        switch (cmd) {
                        case '1': case '2': case '3': case '4': case '5':
                        case '6': case '7': case '8': case '9':
                            {
                                int frames = 1 << (cmd - '1');
                                for (int f = 0; f < frames; f++) {
                                    p8_flip();
                                }
                            }
                            break;
                        case 'c':
                            if (i + 1 < str_len) {
                                int clear_col = hexy(str[++i]);
                                if (clear_col != -1) {
                                    clear_screen(clear_col);
                                    x = 0;
                                    y = 0;
                                    home_x = 0;
                                    home_y = 0;
                                    left_margin = 0;
                                }
                            }
                            break;
                        case 'd': // "\^d"
                            if (i + 1 < str_len) {
                                int df = hexy(str[++i]);
                                if (df != -1)
                                    state.delay_frames = df;
                            }
                            break;
                        case 'g':
                            x = home_x;
                            y = home_y;
                            break;
                        case 'h':
                            home_x = x;
                            home_y = y;
                            break;
                        case 'j':
                            if (i + 2 < str_len) {
                                int jx = hexy(str[++i]);
                                int jy = hexy(str[++i]);
                                if (jx != -1 && jy != -1) {
                                    x = jx * 4;
                                    y = jy * 4;
                                }
                            }
                            break;
                        case 'r':
                            if (i + 1 < str_len) {
                                int wb = hexy(str[++i]);
                                if (wb != -1) {
                                    wrap_boundary = wb * 4;
                                    wrap_enabled = true;
                                }
                            }
                            break;
                        case 's':
                            if (i + 1 < str_len) {
                                int tw = hexy(str[++i]);
                                if (tw != -1)
                                    tab_width = tw;
                            }
                            break;
                        case 'x':
                            if (i + 1 < str_len) {
                                int cw = hexy(str[++i]);
                                if (cw != -1)
                                    state.char_w = cw;
                            }
                            break;
                        case 'y':
                            if (i + 1 < str_len) {
                                int ch = hexy(str[++i]);
                                if (ch != -1)
                                    state.char_h = ch;
                            }
                            break;
                        case 'u':
                            break;
                        case 'w':
                            state.wide = true;
                            break;
                        case 't':
                            state.tall = true;
                            break;
                        case '=':
                            state.stripey = true;
                            break;
                        case 'p':
                            state.wide = true;
                            state.tall = true;
                            state.stripey = true;
                            break;
                        case 'i':
                            state.invert = true;
                            break;
                        case 'b':
                            state.border = !state.border;
                            break;
                        case '#':
                            state.solid_bg = true;
                            break;
                        case '-':
                            if (i + 1 < str_len) {
                                uint8_t flag = str[++i];
                                switch (flag) {
                                case 'w': state.wide = false; break;
                                case 't': state.tall = false; break;
                                case '=': state.stripey = false; break;
                                case 'p':
                                    state.wide = false;
                                    state.tall = false;
                                    state.stripey = false;
                                    break;
                                case 'i': state.invert = false; break;
                                case 'b': state.border = !state.border; break;
                                case '#':
                                    state.solid_bg = false;
                                    break;
                                }
                            }
                            break;
                        case '.':
                        case ',':
                            if (i + 8 < str_len) {
                                bool respect_padding = (cmd == ',');
                                int draw_x = x + (respect_padding && state.border ? 1 : 0);
                                int draw_y = y + (respect_padding && state.border ? 1 : 0);

                                for (int row = 0; row < 8 && i + 1 < str_len; row++) {
                                    uint8_t byte = str[++i];
                                    for (int bit = 0; bit < 8; bit++) {
                                        if (byte & (1 << bit)) {
                                            pixel_set(draw_x + bit, draw_y + row, fg, 0, DRAWTYPE_DEFAULT);
                                        } else if (state.solid_bg) {
                                            pixel_set(draw_x + bit, draw_y + row, bg, 0, DRAWTYPE_DEFAULT);
                                        }
                                    }
                                }
                                x += 8;
                                last_advance = 8;
                            }
                            break;
                        case ':':
                        case ';':
                            if (i + 16 < str_len) {
                                bool respect_padding = (cmd == ';');
                                int draw_x = x + (respect_padding && state.border ? 1 : 0);
                                int draw_y = y + (respect_padding && state.border ? 1 : 0);

                                bool valid_hex = true;
                                for (int row = 0; row < 8 && valid_hex; row++) {
                                    int h1 = hexy(str[i + row*2 + 1]);
                                    int h2 = hexy(str[i + row*2 + 2]);
                                    if (h1 == -1 || h2 == -1) {
                                        valid_hex = false;
                                    }
                                }
                                if (valid_hex) {
                                    for (int row = 0; row < 8; row++) {
                                        uint8_t byte = (hexy(str[i + row*2 + 1]) << 4) | hexy(str[i + row*2 + 2]);
                                        for (int bit = 0; bit < 8; bit++) {
                                            if (byte & (1 << bit)) {
                                                pixel_set(draw_x + bit, draw_y + row, fg, 0, DRAWTYPE_DEFAULT);
                                            } else if (state.solid_bg) {
                                                pixel_set(draw_x + bit, draw_y + row, bg, 0, DRAWTYPE_DEFAULT);
                                            }
                                        }
                                    }
                                    i += 16;
                                    x += 8;
                                    last_advance = 8;
                                }
                            }
                            break;
                        case '@':
                            // \^@addrnnnn[data] — poke nnnn bytes to address addr
                            // addr and nnnn are 4 hex chars each
                            if (i + 8 < str_len) {
                                int addr_h1 = hexy(str[i+1]);
                                int addr_h2 = hexy(str[i+2]);
                                int addr_h3 = hexy(str[i+3]);
                                int addr_h4 = hexy(str[i+4]);
                                int count_h1 = hexy(str[i+5]);
                                int count_h2 = hexy(str[i+6]);
                                int count_h3 = hexy(str[i+7]);
                                int count_h4 = hexy(str[i+8]);
                                if (addr_h1 != -1 && addr_h2 != -1 && addr_h3 != -1 && addr_h4 != -1 &&
                                    count_h1 != -1 && count_h2 != -1 && count_h3 != -1 && count_h4 != -1) {
                                    int addr = (addr_h1 << 12) | (addr_h2 << 8) |
                                               (addr_h3 <<  4) |  addr_h4;
                                    int count = (count_h1 << 12) | (count_h2 << 8) |
                                                (count_h3 <<  4) |  count_h4;
                                    i += 8;
                                    for (int k = 0; k < count && i + 1 < str_len; k++) {
                                        uint8_t byte = str[++i];
                                        if ((unsigned)(addr + k) < MEMORY_SIZE)
                                            m_memory[addr + k] = byte;
                                    }

                                    add_newline = (m_memory[MEMORY_MISCFLAGS] & 0x4) == 0;
                                    wrap_enabled = (m_memory[MEMORY_MISCFLAGS] & 0x80) != 0;
                                    memory_no_scroll = (m_memory[MEMORY_MISCFLAGS] & 0x40) != 0;
                                    do_scroll = !memory_no_scroll && !no_scroll;
                                }
                            }
                            break;
                        case '!':
                            // \^!addr[data] — poke all remaining characters to address addr
                            // addr is 4 hex chars; remaining string bytes are data (not printed)
                            if (i + 4 < str_len) {
                                int addr_h1 = hexy(str[i+1]);
                                int addr_h2 = hexy(str[i+2]);
                                int addr_h3 = hexy(str[i+3]);
                                int addr_h4 = hexy(str[i+4]);
                                if (addr_h1 != -1 && addr_h2 != -1 && addr_h3 != -1 && addr_h4 != -1) {
                                    int addr = (addr_h1 << 12) | (addr_h2 << 8) |
                                               (addr_h3 <<  4) |  addr_h4;
                                    i += 4;
                                    int k = 0;
                                    while (i + 1 < str_len) {
                                        uint8_t byte = str[++i];
                                        if ((unsigned)(addr + k) < MEMORY_SIZE)
                                            m_memory[addr + k] = byte;
                                        k++;
                                    }
                                }
                            }
                            break;
                        case 'o':
                            if (i + 3 < str_len) {
                                uint8_t colour_char = str[++i];
                                int mask_h = hexy(str[++i]);
                                int mask_l = hexy(str[++i]);

                                if (mask_h != -1 && mask_l != -1) {
                                    state.outline_mask = (mask_h << 4) | mask_l;

                                    if (colour_char == '$') {
                                        state.outline_colour = fg;
                                        state.outline_skip_interior = false;
                                    } else if (colour_char == '!') {
                                        state.outline_colour = fg;
                                        state.outline_skip_interior = true;
                                    } else {
                                        int oc = hexy(colour_char);
                                        if (oc != -1)
                                            state.outline_colour = oc;
                                        state.outline_skip_interior = false;
                                    }

                                    state.outline_enabled = (state.outline_mask != 0);
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case 7: // "\a"
                    if (i + 1 < str_len) {
                        int h1 = hexy(str[i + 1]);
                        if (h1 != -1) {
                            if (i + 2 < str_len) {
                                int h2 = hexy(str[i + 2]);
                                if (h2 != -1) {
                                    int sfx_index = (h1 << 4) | h2;
#ifdef ENABLE_AUDIO
                                    audio_sound(sfx_index, -1, 0, 32);
#endif
                                    i += 2;
                                }
                            }
                        }
                    } else {
                        // TODO: note data support
                    }
                    break;
                case 8: // "\b"
                    x -= last_advance;
                    break;
                case 9: // "\t"
                    x = ((x / tab_width) + 1) * tab_width;
                    break;
                case 0xa: // "\n"
                    x = left_margin;
                    y += state.char_h * (state.tall ? 2 : 1);
                    if (do_scroll)
                        y = scroll_newline(y);
                    break;
                case 0xb: // "\v"
                    if (i + 2 < str_len) {
                        int offset_code = hexy(str[++i]);
                        if (offset_code != -1) {
                            uint8_t decoration_char = str[++i];

                            int offset_x = (offset_code % 4) - 2;
                            int offset_y = (offset_code / 4) - 8;

                            int save_x = x;
                            int save_y = y;

                            int dec_x = save_x - last_advance + offset_x;
                            int dec_y = save_y + offset_y;

                            uint8_t symbol_length = 0;
                            int dec_index = get_p8_symbol((const char*)&decoration_char, 1, &symbol_length);
                            if (dec_index == -1) {
                                dec_index = decoration_char;
                            }

                            if (dec_index >= 0x20 && dec_index < 0x7F) {
                                draw_char(dec_index, dec_x, dec_y, fg);
                            }
                        }
                    }
                    break;
                case 0xc: // "\f"
                    if (i + 1 < str_len) {
                        int fc = hexy(str[++i]);
                        if (fc != -1) {
                            fg = fc;
                            m_memory[MEMORY_PENCOLOR] = fg;
                        }
                    }
                    break;
                case 0xd: // "\r"
                    x = left_margin;
                    break;
                case 0xe: // "\014"
                    state.use_custom_font = true;
                    if (m_memory[MEMORY_FONT + 6] != 0) {
                        tab_width = m_memory[MEMORY_FONT + 6];
                    }
                    if (m_memory[MEMORY_FONT + 2] != 0) {
                        state.char_h = m_memory[MEMORY_FONT + 2];
                    }
                    break;
                case 0xf: // "\015"
                    state.use_custom_font = false;
                    break;
                default:
                break;
                }
            }
            else
            {
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
                    else
                        index = character;
                }

                if (index >= 0) {
                    int base_char_width = state.use_custom_font ? get_custom_font_char_width(index) : (index >= 0x80 ? state.char_w2 : state.char_w);
                    int char_width = base_char_width * (state.wide ? 2 : 1);

                    bool use_styled = state.wide || state.tall || state.invert || !state.border || state.outline_enabled || state.use_custom_font || (state.char_w != GLYPH_WIDTH) || (state.char_w2 != GLYPH_WIDTH) || (state.char_h != GLYPH_HEIGHT);

                    if (wrap_enabled && x + char_width > wrap_boundary) {
                        x = left_margin;
                        y += state.char_h * (state.tall ? 2 : 1);
                    }
                    if (do_scroll)
                        y = scroll(y, state.char_h * (state.tall ? 2 : 1));

                    int draw_bg = state.solid_bg ? bg : -1;
                    if (use_styled) {
                        draw_char_styled(index, x, y, fg, draw_bg, &state);
                    } else if (draw_bg != -1) {
                        int char_height = state.char_h * (state.tall ? 2 : 1);
                        draw_rectfill(x, y, x + char_width - 1, y + char_height - 1, draw_bg, 0);
                        draw_char(index, x, y, fg);
                    } else {
                        draw_char(index, x, y, fg);
                    }

                    x += char_width;
                    last_advance = char_width;

                    for (int f = 0; f < state.delay_frames; f++) {
                        p8_flip();
                    }

                    if (x > right) right = x;
                    int char_bottom =  y + state.char_h * (state.tall ? 2 : 1);
                    if (char_bottom > bottom) bottom = char_bottom;
                }
            }
        }
    }

    if (add_newline) {
        x = left_margin;
        y += state.char_h * (state.tall ? 2 : 1);
        if (do_scroll)
            y = scroll_newline(y);
    }

    if (out_left)
        *out_left = x;
    if (out_top)
        *out_top = y;
    if (out_right)
        *out_right = right;
    if (out_bottom)
        *out_bottom = bottom;
}

#endif // P8_PRINT_HELPER_H
