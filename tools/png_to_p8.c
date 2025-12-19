/*
 * png_to_p8.c
 *
 * Convert a PICO-8 .p8.png file to a .p8 file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <limits.h>

#include "p8_emu.h"
#include "p8_lua_helper.h"
#include "p8_parser.h"
#include "p8_symbols.h"
#include "lodepng.h"

static inline const p8_symbol_t *get_p8_encoding(uint8_t index)
{
    int p8_symbols_len = sizeof(p8_symbols) / sizeof(p8_symbol_t);
    for (int i = 0; i < p8_symbols_len; i++) {
        if (p8_symbols[i].index == index)
            return &p8_symbols[i];
    }
    return NULL;
}

char *convert_p8scii_to_utf8(const char *p8scii_str)
{
    if (!p8scii_str) return NULL;

    size_t len = strlen(p8scii_str);
    size_t utf8_size = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)p8scii_str[i];

        const p8_symbol_t *sym = get_p8_encoding(c);
        if (sym)
            utf8_size += sym->length;
        else
            utf8_size += 1;
    }

    char *utf8_str = malloc(utf8_size + 1);
    if (!utf8_str) return NULL;

    char *out = utf8_str;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)p8scii_str[i];

        const p8_symbol_t *sym = get_p8_encoding(c);
        if (sym) {
            memcpy(out, sym->encoding, sym->length);
            out += sym->length;
        } else {
            *out++ = c;
        }
    }
    *out = '\0';

    return utf8_str;
}

void write_gfx_section(FILE *out, uint8_t *data, int size)
{
    for (int i = 0; i < size; i++) {
        uint8_t byte = data[i];
        fprintf(out, "%x%x", byte & 0xF, (byte >> 4) & 0xF);
        if ((i + 1) % 64 == 0) {
            fprintf(out, "\n");
        }
    }
}

void write_gff_section(FILE *out, uint8_t *data, int size)
{
    for (int i = 0; i < size; i++) {
        fprintf(out, "%02x", data[i]);
        if ((i + 1) % 128 == 0 && i + 1 < size)
            fprintf(out, "\n");
    }
    fprintf(out, "\n");
}

void write_map_section(FILE *out, uint8_t *data, int size)
{
    for (int i = 0; i < size; i++) {
        fprintf(out, "%02x", data[i]);
        if ((i + 1) % 128 == 0 && i + 1 < size)
            fprintf(out, "\n");
    }
    fprintf(out, "\n");
}

void write_sfx_section(FILE *out, uint8_t *memory)
{
    for (int sfx = 0; sfx < 64; sfx++) {
        uint8_t *sfx_data = memory + MEMORY_SFX + sfx * 68;

        // Write SFX header
        fprintf(out, "%02x%02x%02x%02x", sfx_data[64], sfx_data[65], sfx_data[66], sfx_data[67]);

        for (int note = 0; note < 32; note++) {
            uint16_t note_data = sfx_data[note * 2] | (sfx_data[note * 2 + 1] << 8);

            uint8_t pitch = note_data & 0x3F;
            uint8_t waveform = (note_data >> 6) & 0x7;
            uint8_t volume = (note_data >> 9) & 0x7;
            uint8_t effect = (note_data >> 12) & 0x7;
            uint8_t custom = (note_data >> 15) & 0x1;

            uint8_t waveform_p8 = waveform | (custom << 3);

            // 5 hex digits: pp w v e
            uint32_t p8_note = (pitch << 12) | (waveform_p8 << 8) | (volume << 4) | effect;
            fprintf(out, "%05x", p8_note);
        }
        fprintf(out, "\n");
    }
}

void write_music_section(FILE *out, uint8_t *memory)
{
    // Find the last non-empty pattern - PICO-8 omits trailing empty patterns
    int last_pattern = -1;
    for (int pattern = 0; pattern < 64; pattern++) {
        uint8_t *pattern_data = memory + MEMORY_MUSIC + pattern * 4;
        if (!(pattern_data[0] == 0x41 && pattern_data[1] == 0x42 &&
              pattern_data[2] == 0x43 && pattern_data[3] == 0x44)) {
            last_pattern = pattern;
        }
    }

    for (int pattern = 0; pattern <= last_pattern; pattern++) {
        uint8_t *pattern_data = memory + MEMORY_MUSIC + pattern * 4;

        uint8_t flags = 0;
        if (pattern_data[0] & 0x80) flags |= 0x01; // begin loop
        if (pattern_data[1] & 0x80) flags |= 0x02; // end loop
        if (pattern_data[2] & 0x80) flags |= 0x04; // stop at end

        fprintf(out, "%02x ", flags);

        for (int ch = 0; ch < 4; ch++) {
            uint8_t byte = pattern_data[ch] & 0x7F;
            fprintf(out, "%02x", byte);
        }
        fprintf(out, "\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <input.p8.png> [output.p8]\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = NULL;
    char *default_output_file = NULL;
    int exit_status = 0;

    if (argc == 3) {
        output_file = argv[2];
    } else {
        default_output_file = strdup(input_file);

        size_t len = strlen(default_output_file);
        if (len > 4 && strcmp(default_output_file + len - 4, ".png") == 0)
            default_output_file[len - 4] = '\0';

        output_file = default_output_file;
    }

    FILE *png_file = fopen(input_file, "rb");
    if (!png_file) {
        fprintf(stderr, "Error: Could not open input file %s\n", input_file);
        return 1;
    }
    fseek(png_file, 0, SEEK_END);
    long png_file_size = ftell(png_file);
    fseek(png_file, 0, SEEK_SET);
    uint8_t *png_file_data = (uint8_t *)malloc(png_file_size);
    if (!png_file_data) {
        fprintf(stderr, "Error: Could not allocate memory for PNG file\n");
        return 1;
    }
    fread(png_file_data, 1, png_file_size, png_file);
    fclose(png_file);

    uint8_t *png_pixels = NULL;
    unsigned png_width = 0, png_height = 0;
    unsigned png_error = lodepng_decode32(&png_pixels, &png_width, &png_height, png_file_data, png_file_size);
    free(png_file_data);

    if (png_error) {
        fprintf(stderr, "Error decoding PNG: %s\n", lodepng_error_text(png_error));
        return 1;
    }

    uint8_t *memory = calloc(0x8000, 1);
    if (!memory) {
        fprintf(stderr, "Error: Could not allocate memory\n");
        return 1;
    }

    uint8_t *label_image = calloc(0x4000, 1);
    if (!label_image) {
        fprintf(stderr, "Error: Could not allocate label image memory\n");
        return 1;
    }
    memset(label_image, 0, 0x4000);

    const char *lua_script = NULL;
    uint8_t *file_buffer = NULL;

    parse_cart_file(input_file, memory, &lua_script, &file_buffer, label_image);

    if (!lua_script) {
        fprintf(stderr, "Error: Could not extract Lua code from %s\n", input_file);
        return 1;
    }

    FILE *out = fopen(output_file, "w");
    if (!out) {
        fprintf(stderr, "Error: Could not open output file %s\n", output_file);
        return 1;
    }

    fprintf(out, "pico-8 cartridge // http://www.pico-8.com\n");
    fprintf(out, "version 43\n");

    fprintf(out, "__lua__\n");

    char *utf8_lua = convert_p8scii_to_utf8(lua_script);
    size_t len = strlen(utf8_lua);
    fwrite(utf8_lua, 1, len, out);
    if (len > 0 && utf8_lua[len - 1] != '\n')
        fprintf(out, "\n");
    free(utf8_lua);

    fprintf(out, "__gfx__\n");
    write_gfx_section(out, memory + MEMORY_SPRITES, MEMORY_SPRITES_SIZE + MEMORY_SPRITES_MAP_SIZE);

    fprintf(out, "__label__\n");
    const char hex_chars[] = "0123456789abcdefghijklmnopqrstuv";
    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            uint8_t color = label_image[y * 128 + x];
            fprintf(out, "%c", hex_chars[color]);
        }
        fprintf(out, "\n");
    }
    fprintf(out, "\n");

    fprintf(out, "__gff__\n");
    write_gff_section(out, memory + MEMORY_SPRITEFLAGS, MEMORY_SPRITEFLAGS_SIZE);

    fprintf(out, "__map__\n");
    write_map_section(out, memory + MEMORY_MAP, MEMORY_MAP_SIZE);

    fprintf(out, "__sfx__\n");
    write_sfx_section(out, memory);

    fprintf(out, "__music__\n");
    write_music_section(out, memory);
    fprintf(out, "\n");

    fclose(out);

    return exit_status;
}
