/*
 * p8_parser.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include "lodepng.h"
#include "p8_emu.h"
#include "pico8.h"
#include "p8_lua_helper.h"

#define RAW_DATA_LENGTH 0x4300
#define IMAGE_WIDTH 160
#define IMAGE_HEIGHT 205

enum P8Type
{
    P8TYPE_HEADER,
    P8TYPE_LUA,
    P8TYPE_GFX_4BIT,
    P8TYPE_GFX_8BIT,
    P8TYPE_GFF,
    P8TYPE_LABEL,
    P8TYPE_MAP,
    P8TYPE_SFX,
    P8TYPE_MUSIC,
    P8TYPE_COUNT,
};

static const char *m_p8_name[] = {
    NULL,
    "__lua__",
    "__gfx__",
    "__gfx8__",
    "__gff__",
    "__label__",
    "__map__",
    "__sfx__",
    "__music__",
};

static int m_p8_mem_offset[] = {
    0,
    0,
    MEMORY_SPRITES,
    0,
    MEMORY_SPRITEFLAGS,
    0,
    MEMORY_MAP,
    MEMORY_SFX,
    MEMORY_MUSIC,
};

#if defined(_WIN32) || defined(__MINGW32__)
static char* strsep_portable(char **stringp, const char *delim)
{
    char *start, *p;
    if (stringp == NULL || *stringp == NULL) return NULL;

    start = *stringp;
    for (p = start; *p; ++p) {
        const char *d;
        for (d = delim; *d; ++d) {
            if (*p == *d) {
                *p = '\0';
                *stringp = p + 1;
                return start;
            }
        }
    }
    *stringp = NULL;
    return start;
}
#define strsep strsep_portable
#endif

int parse_cart_ram(uint8_t *buffer, int size, uint8_t *memory, const char **lua_script, uint8_t **decompression_buffer, uint8_t *label_image);
int parse_cart_file(const char *file_name, uint8_t *memory, const char **lua_script, uint8_t **file_buffer, uint8_t *label_image);
static int parse_p8_ram(const char *file_name, uint8_t *buffer, int size, uint8_t *memory, const char **lua_script, uint8_t *label_image);
static int parse_png_ram(const char *file_name, uint8_t *buffer, int size, uint8_t *memory, const char **lua_script, uint8_t **decompression_buffer, uint8_t *label_image);
static char *process_includes(const char *lua_script, const char *cart_dir);
static void convert_utf8_to_p8scii(uint8_t *buffer, size_t len);

static uint8_t PNG_SIGNATURE[8] = {137, 80, 78, 71, 13, 10, 26, 10};

static int parse_cart_ram0(const char *file_name, uint8_t *buffer, int size, uint8_t *memory, const char **lua_script, uint8_t **decompression_buffer, uint8_t *label_image)
{
    if (size >= 8 &&
        memcmp(buffer, PNG_SIGNATURE, 8) == 0) {
        return parse_png_ram(file_name, buffer, size, memory, lua_script, decompression_buffer, label_image);
    } else {
        *decompression_buffer = NULL;
        return parse_p8_ram(file_name, buffer, size, memory, lua_script, label_image);
    }
}

int parse_cart_ram(uint8_t *buffer, int size, uint8_t *memory, const char **lua_script, uint8_t **decompression_buffer, uint8_t *label_image)
{
    return parse_cart_ram0(NULL, buffer, size, memory, lua_script, decompression_buffer, label_image);
}

static char *process_includes(const char *lua_script, const char *cart_dir)
{
    // Quick scan: any #include lines present?
    const char *scan = lua_script;
    bool has_includes = false;
    while ((scan = strstr(scan, "#include")) != NULL) {
        // Must be at start of line (or start of string)
        if (scan == lua_script || scan[-1] == '\n') {
            has_includes = true;
            break;
        }
        scan += 8;
    }
    if (!has_includes)
        return NULL;

    // Build expanded buffer
    size_t capacity = strlen(lua_script) * 2;
    size_t length = 0;
    char *result = malloc(capacity + 1);
    if (!result)
        return NULL;

    const char *p = lua_script;
    while (*p) {
        // Find end of current line
        const char *eol = strchr(p, '\n');
        int line_len = eol ? (int)(eol - p) : (int)strlen(p);

        // Check for #include at start of line
        if (strncmp(p, "#include ", 9) == 0) {
            // Extract filename (skip "#include ")
            const char *fname_start = p + 9;
            int fname_len = line_len - 9;
            // Trim trailing whitespace/CR
            while (fname_len > 0 && (fname_start[fname_len - 1] == ' ' ||
                                     fname_start[fname_len - 1] == '\r'))
                fname_len--;

            if (fname_len > 0) {
                // Build full path relative to the cart file's directory
                char include_path[PATH_MAX];
                snprintf(include_path, sizeof(include_path), "%s/%.*s",
                         cart_dir, fname_len, fname_start);

                FILE *inc = fopen(include_path, "rb");
                if (inc) {
                    fseek(inc, 0, SEEK_END);
                    long inc_size = ftell(inc);
                    rewind(inc);

                    // Grow result buffer if needed
                    while (length + inc_size + 1 > capacity) {
                        capacity *= 2;
                        char *new_result = realloc(result, capacity + 1);
                        if (!new_result) {
                            fclose(inc);
                            free(result);
                            return NULL;
                        }
                        result = new_result;
                    }

                    fread(result + length, 1, inc_size, inc);
                    convert_utf8_to_p8scii((uint8_t *)(result + length), inc_size);
                    length += strlen(result + length);
                    fclose(inc);

                    // Ensure included content ends with newline
                    if (length > 0 && result[length - 1] != '\n')
                        result[length++] = '\n';
                } else {
                    fprintf(stderr, "Warning: #include file not found: %s\n", include_path);
                }
            }
        } else {
            // Copy line as-is (including newline)
            int copy_len = eol ? line_len + 1 : line_len;
            while (length + copy_len + 1 > capacity) {
                capacity *= 2;
                char *new_result = realloc(result, capacity + 1);
                if (!new_result) {
                    free(result);
                    return NULL;
                }
                result = new_result;
            }
            memcpy(result + length, p, copy_len);
            length += copy_len;
        }

        p += line_len;
        if (eol)
            p++; // skip newline
    }

    result[length] = '\0';
    return result;
}

int parse_cart_file(const char *file_name, uint8_t *memory, const char **lua_script, uint8_t **file_buffer, uint8_t *label_image)
{
    FILE *file = fopen(file_name, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", file_name);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

#ifndef OS_FREERTOS
    *file_buffer = (uint8_t *)malloc(file_size + 1);
#else
    *file_buffer = (uint8_t *)rh_malloc(file_size + 1);
#endif

    fread(*file_buffer, 1, file_size, file);

    fclose(file);

    (*file_buffer)[file_size] = '\0';

    uint8_t *decompression_buffer = NULL;
    if (parse_cart_ram0(file_name, (uint8_t *)*file_buffer, (int)file_size, memory, lua_script, &decompression_buffer, label_image) != 0) {
#ifdef OS_FREERTOS
        rh_free(*file_buffer);
#else
        free(*file_buffer);
#endif
        return -1;
    }
    if (decompression_buffer) {
#ifdef OS_FREERTOS
        rh_free(*file_buffer);
#else
        free(*file_buffer);
#endif
        *file_buffer = decompression_buffer;
    }

    // Process #include directives in the Lua section
    if (*lua_script && file_name) {
        const char *last_slash = strrchr(file_name, '/');
        char cart_dir[PATH_MAX];
        if (last_slash) {
            size_t dir_len = last_slash - file_name;
            if (dir_len >= sizeof(cart_dir))
                dir_len = sizeof(cart_dir) - 1;
            memcpy(cart_dir, file_name, dir_len);
            cart_dir[dir_len] = '\0';
        } else {
            strcpy(cart_dir, ".");
        }

        char *expanded = process_includes(*lua_script, cart_dir);
        if (expanded) {
#ifdef OS_FREERTOS
            rh_free(*file_buffer);
#else
            free(*file_buffer);
#endif
            *file_buffer = (uint8_t *)expanded;
            *lua_script = expanded;
        }
    }

    return 0;
}

static void convert_utf8_to_p8scii(uint8_t *buffer, size_t len)
{
    uint8_t *read_ptr = buffer;
    uint8_t *write_ptr = buffer;
    uint8_t *end_ptr = buffer + len;

    while (read_ptr < end_ptr) {
        if (*read_ptr >= 0x80) {
            uint8_t symbol_length;
            int p8_index = get_p8_symbol((const char *)read_ptr, end_ptr - read_ptr, &symbol_length);
            
            if (p8_index >= 0) {
                *write_ptr++ = (uint8_t)p8_index;
                read_ptr += symbol_length;
            } else {
                *write_ptr++ = *read_ptr++;
            }
        } else {
            *write_ptr++ = *read_ptr++;
        }
    }

    *write_ptr = '\0';
}

void hex_to_bytes(uint8_t *memory, char *str, int str_len, int *write_length)
{
    *write_length = 0;

    if (str == NULL)
        return;

    if (str_len == 0)
        return;

    int read_offset = 0;
    int write_offset = 0;

    while (read_offset < str_len)
    {
        if (!isxdigit(str[read_offset]))
        {
            read_offset++;
            continue;
        }
        unsigned int v;
        sscanf(str + read_offset, "%2x", &v);
        memory[write_offset] = (uint8_t)v;
        read_offset += 2;
        write_offset++;
    }

    *write_length = write_offset;
}

void read_sfx(uint8_t *dest, uint8_t *src, int read_length, int *write_length)
{
    int read_offset = 0;
    int write_offset = 0;

    while (read_offset < read_length)
    {
        uint8_t byte_0 = src[read_offset++];
        uint8_t byte_1 = src[read_offset++];
        uint8_t byte_2 = src[read_offset++];
        uint8_t byte_3 = src[read_offset++];

        for (int i = 0; i < 16; i++)
        {
            uint8_t byte_a = src[read_offset++];
            uint8_t byte_b = src[read_offset++];
            uint8_t byte_c = src[read_offset++];
            uint8_t byte_d = src[read_offset++];
            uint8_t byte_e = src[read_offset++];

            uint8_t note1_pitch = byte_a;
            uint8_t note1_waveform = byte_b >> 4;
            uint8_t note1_volume = byte_b & 0xF;
            uint8_t note1_effect = byte_c >> 4;

            uint8_t note2_pitch = (byte_c << 4) | (byte_d >> 4);
            uint8_t note2_waveform = byte_d & 0xF;
            uint8_t note2_volume = byte_e >> 4;
            uint8_t note2_effect = byte_e & 0xF;

            uint16_t note1 = (note1_waveform > 7 ? 1 << 15 : 0) |
                             (note1_effect << 12) | (note1_volume << 9) |
                             ((note1_waveform > 7 ? note1_waveform - 8 : note1_waveform) << 6) |
                             note1_pitch;
            uint16_t note2 = (note2_waveform > 7 ? 1 << 15 : 0) |
                             (note2_effect << 12) | (note2_volume << 9) |
                             ((note2_waveform > 7 ? note2_waveform - 8 : note2_waveform) << 6) |
                             note2_pitch;

            dest[write_offset++] = note1;
            dest[write_offset++] = note1 >> 8;
            dest[write_offset++] = note2;
            dest[write_offset++] = note2 >> 8;
        }

        dest[write_offset++] = byte_0; // Editor Mode
        dest[write_offset++] = byte_1; // Speed
        dest[write_offset++] = byte_2; // Loop Start
        dest[write_offset++] = byte_3; // Loop End
    }

    *write_length = write_offset;
}

void read_music(uint8_t *dest, uint8_t *src, int read_length, int *write_length)
{
    int read_offset = 0;
    int write_offset = 0;

    while (read_offset < read_length)
    {
        uint8_t flags_byte = src[read_offset++];
        uint8_t effect_id1 = src[read_offset++];
        uint8_t effect_id2 = src[read_offset++];
        uint8_t effect_id3 = src[read_offset++];
        uint8_t effect_id4 = src[read_offset++];

        int begin_patter_loop = flags_byte & (1 << 0);
        int end_pattern_loop = flags_byte & (1 << 1);
        int stop_at_end_of_pattern = flags_byte & (1 << 2);

        bool channel1_silence = (effect_id1 & 0x40) != 0;
        bool channel2_silence = (effect_id2 & 0x40) != 0;
        bool channel3_silence = (effect_id3 & 0x40) != 0;
        bool channel4_silence = (effect_id4 & 0x40) != 0;

        dest[write_offset++] = (channel1_silence ? 1 << 6 : effect_id1 & 0x7F) | (begin_patter_loop ? 1 << 7 : 0);
        dest[write_offset++] = (channel2_silence ? 1 << 6 : effect_id2 & 0x7F) | (end_pattern_loop ? 1 << 7 : 0);
        dest[write_offset++] = (channel3_silence ? 1 << 6 : effect_id3 & 0x7F) | (stop_at_end_of_pattern ? 1 << 7 : 0);
        dest[write_offset++] = (channel4_silence ? 1 << 6 : effect_id4 & 0x7F);
    }

    *write_length = write_offset;
}

int parse_p8_ram(const char *file_name, uint8_t *buffer, int size, uint8_t *memory, const char **lua_script, uint8_t *label_image)
{
    static uint8_t tmpbuf[180];
    char *line;
    char *rest = (char *)buffer;
    int lua_start = 0, lua_end = 0;
    int p8_type = P8TYPE_HEADER;
    int file_offset = 0;
    int read_offset = 0;
    int write_offset = 0;
    int read_length = 0;
    int write_length = 0;

    if (label_image)
        memset(label_image, 0, 0x4000);

    while ((line = strsep(&rest, "\n")) != NULL)
    {
        int token_found = 0;
        int line_length = rest ? (rest - line) : (size - file_offset);

        if (file_offset > 0)
            line[-1] = '\n';

        file_offset += line_length;

        for (int i = P8TYPE_LUA; i < P8TYPE_COUNT; i++)
        {
            if (strncmp(line, m_p8_name[i], strlen(m_p8_name[i])) == 0)
            {
                p8_type = i;
                read_offset = 0;
                write_offset = 0;
                token_found = 1;
                break;
            }
        }

        if (token_found)
        {
            if (p8_type == P8TYPE_LUA)
            {
                lua_start = file_offset;
                lua_end = file_offset;
            }
            continue;
        }

        switch (p8_type)
        {
        case P8TYPE_HEADER:
            break;
        case P8TYPE_LUA:
        {
            lua_end += line_length;
            break;
        }
        case P8TYPE_GFX_4BIT:
        // case P8TYPE_GFX_8BIT:
        case P8TYPE_GFF:
        case P8TYPE_MAP:
        {
            uint8_t *write_mem = memory + m_p8_mem_offset[p8_type] + write_offset;
            hex_to_bytes(write_mem, line, line_length-1, &write_length);

            write_offset += write_length;
            break;
        }
        case P8TYPE_LABEL:
        {
            if (label_image) {
                const char *hex_chars = "0123456789abcdefghijklmnopqrstuv";
                int x = 0;
                for (int i = 0; i < line_length - 1 && x < 128; i++) {
                    char c = line[i];
                    const char *pos = strchr(hex_chars, c);
                    if (pos) {
                        label_image[write_offset * 128 + x] = (uint8_t)(pos - hex_chars);
                        x++;
                    }
                }
                if (x > 0) write_offset++;
            }
            break;
        }
        case P8TYPE_SFX:
        {
            uint8_t *write_mem = memory + MEMORY_SFX + write_offset;

            hex_to_bytes(tmpbuf, line, line_length-1, &read_length);

            if (read_length > 0)
                read_sfx(write_mem, tmpbuf, read_length, &write_length);

            read_offset += read_length;
            write_offset += write_length;
            break;
        }
        case P8TYPE_MUSIC:
        {
            uint8_t *write_mem = memory + MEMORY_MUSIC + write_offset;

            hex_to_bytes(tmpbuf, line, line_length-1, &read_length);

            if (read_length > 0)
                read_music(write_mem, tmpbuf, read_length, &write_length);

            read_offset += read_length;
            write_offset += write_length;
            break;
        }
        }
    }

    for (int i = MEMORY_SPRITES; i < MEMORY_SPRITES + MEMORY_SPRITES_SIZE; i++)
        memory[i] = NIBBLE_SWAP(memory[i]);

    for (int i = MEMORY_SPRITES_MAP; i < MEMORY_SPRITES_MAP + MEMORY_SPRITES_MAP_SIZE; i++)
        memory[i] = NIBBLE_SWAP(memory[i]);

    buffer[lua_end] = '\0';

    convert_utf8_to_p8scii(&buffer[lua_start], lua_end - lua_start);

    *lua_script = (const char *) &buffer[lua_start];
    return 0;
}

#define PNG_WIDTH 160
#define PNG_HEIGHT 205

int parse_png_ram(const char *file_name, uint8_t *buffer, int file_size, uint8_t *memory, const char **lua_script, uint8_t **decompression_buffer, uint8_t *label_image)
{
    *lua_script = NULL;
    *decompression_buffer = NULL;
    uint8_t *px_buffer = NULL;
    unsigned width = 0, height = 0;
    unsigned ret = lodepng_decode32(&px_buffer, &width, &height, buffer, file_size);
    if (ret != 0) {
        fprintf(stderr, "%s\n", lodepng_error_text(ret));
        return -1;
    }
    if (width != PNG_WIDTH || height != PNG_HEIGHT) {
        if (file_name)
            fprintf(stderr, "%s: ", file_name);
        fprintf(stderr, "PNG has wrong size: %dx%d (expected 160x205)\n", width, height);
        return -1;
    }

    if (label_image) {
        // 32-colour extended palette (R, G, B) with the two least significant bits masked off
        static const uint8_t palette[32][3] = {
            {0, 0, 0}, {28, 40, 80}, {124, 36, 80}, {0, 132, 80},
            {168, 80, 52}, {92, 84, 76}, {192, 192, 196}, {252, 240, 232},
            {252, 0, 76}, {252, 160, 0}, {252, 236, 36}, {0, 228, 52},
            {40, 172, 252}, {128, 116, 156}, {252, 116, 168}, {252, 204, 168},
            {40, 24, 20}, {16, 28, 52}, {64, 32, 52}, {16, 80, 88},
            {116, 44, 40}, {72, 48, 56}, {160, 136, 120}, {240, 236, 124},
            {188, 16, 80}, {252, 108, 36}, {168, 228, 44}, {0, 180, 64},
            {4, 88, 180}, {116, 68, 100}, {252, 108, 88}, {252, 156, 128}
        };

        const int label_left = 16;
        const int label_top = 24;

        for (int y = 0; y < 128; y++) {
            for (int x = 0; x < 128; x += 2) {
                int left_offset = ((label_top + y) * PNG_WIDTH + (label_left + x)) << 2;
                int right_offset = ((label_top + y) * PNG_WIDTH + (label_left + x + 1)) << 2;

                uint8_t left_r = px_buffer[left_offset] & 0xFC;
                uint8_t left_g = px_buffer[left_offset + 1] & 0xFC;
                uint8_t left_b = px_buffer[left_offset + 2] & 0xFC;

                uint8_t right_r = px_buffer[right_offset] & 0xFC;
                uint8_t right_g = px_buffer[right_offset + 1] & 0xFC;
                uint8_t right_b = px_buffer[right_offset + 2] & 0xFC;

                // Find matching palette colour index for each pixel
                uint8_t left_color = 0;
                for (int i = 0; i < 32; i++) {
                    if (palette[i][0] == left_r &&
                        palette[i][1] == left_g &&
                        palette[i][2] == left_b) {
                        left_color = i;
                        break;
                    }
                }

                uint8_t right_color = 0;
                for (int i = 0; i < 32; i++) {
                    if (palette[i][0] == right_r &&
                        palette[i][1] == right_g &&
                        palette[i][2] == right_b) {
                        right_color = i;
                        break;
                    }
                }

                label_image[y * 128 + x] = left_color;
                label_image[y * 128 + x + 1] = right_color;
            }
        }
    }

    uint8_t *px_buffer_in = px_buffer;
    uint8_t *byte_buffer = px_buffer;
    uint8_t *byte_buffer_out = byte_buffer;
    for (unsigned i=0;i<PNG_WIDTH*PNG_HEIGHT;++i) {
        *byte_buffer_out = ((px_buffer_in[3] & 0x3) << 6) | ((px_buffer_in[0] & 0x3) << 4) | ((px_buffer_in[1] & 0x3) << 2) | (px_buffer_in[2] & 0x3);
        px_buffer_in += 4;
        byte_buffer_out++;
    }
    memcpy(memory, byte_buffer, CART_MEMORY_SIZE);
    *decompression_buffer = malloc(0x20001);
    pico8_code_section_decompress(byte_buffer + CART_MEMORY_SIZE, *decompression_buffer, 0x20000);
    *lua_script = (char *)*decompression_buffer;

    return 0;
}
