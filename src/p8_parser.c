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
#include "p8_emu.h"

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

void parse_cart_ram(uint8_t *buffer, int size, uint8_t *memory, char **lua_script);
void parse_cart_file(char *file_name, uint8_t *memory, char **lua_script);
void parse_p8_ram(uint8_t *buffer, int size, uint8_t *memory, char **lua_script);
void parse_p8_file(char *file_name, uint8_t *memory, char **lua_script);
void parse_png(char *file_name, uint8_t *memory, char **lua_script);

void parse_cart_ram(uint8_t *buffer, int size, uint8_t *memory, char **lua_script)
{
    parse_p8_ram(buffer, size, memory, lua_script);
}

void parse_cart_file(char *file_name, uint8_t *memory, char **lua_script)
{
    char extension[5];
    strcpy(extension, strrchr(file_name, '.') + 1);

    if (strcasecmp(extension, "png") == 0)
        parse_png(file_name, memory, lua_script);
    else
        parse_p8_file(file_name, memory, lua_script);
}

void hex_to_bytes(uint8_t *memory, char *value, int *write_length)
{
    *write_length = 0;

    if (value == NULL)
        return;

    int length = strlen(value);

    if (length == 0)
        return;

    int read_offset = 0;
    int write_offset = 0;

    while (read_offset < length)
    {
        if (!isxdigit(value[read_offset]))
        {
            read_offset++;
            continue;
        }
        unsigned int v;
        sscanf(value + read_offset, "%2x", &v);
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

        bool channel1_silence = (effect_id1 == 0x41) || (effect_id2 == 0x41) || (effect_id3 == 0x41) || (effect_id4 == 0x41);
        bool channel2_silence = (effect_id1 == 0x42) || (effect_id2 == 0x42) || (effect_id3 == 0x42) || (effect_id4 == 0x42);
        bool channel3_silence = (effect_id1 == 0x43) || (effect_id2 == 0x43) || (effect_id3 == 0x43) || (effect_id4 == 0x43);
        bool channel4_silence = (effect_id1 == 0x44) || (effect_id2 == 0x44) || (effect_id3 == 0x44) || (effect_id4 == 0x44);

        dest[write_offset++] = (channel1_silence ? 1 << 6 : effect_id1 & 0x7F) | (begin_patter_loop ? 1 << 7 : 0);
        dest[write_offset++] = (channel2_silence ? 1 << 6 : effect_id2 & 0x7F) | (end_pattern_loop ? 1 << 7 : 0);
        dest[write_offset++] = (channel3_silence ? 1 << 6 : effect_id3 & 0x7F) | (stop_at_end_of_pattern ? 1 << 7 : 0);
        dest[write_offset++] = (channel4_silence ? 1 << 6 : effect_id4 & 0x7F);
    }

    *write_length = write_offset;
}

void parse_p8_ram(uint8_t *buffer, int size, uint8_t *memory, char **lua_script)
{
    char *line;
    char *rest = (char *)buffer;
    char *script = *lua_script;
    int p8_type = P8TYPE_HEADER;
    int read_offset = 0;
    int write_offset = 0;
    int read_length = 0;
    int write_length = 0;

    while ((line = strsep(&rest, "\n")) != NULL)
    {
        int token_found = 0;

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
            continue;
        }

        switch (p8_type)
        {
        case P8TYPE_HEADER:
            break;
        case P8TYPE_LUA:
        {
            int line_length = strlen(line) + 1;
            read_length += line_length;

            if (read_length >= MEMORY_LUA_SIZE)
            {
                fprintf(stderr, "Lua code too long!\n");
                exit(EXIT_FAILURE);
            }

            strcat(script, line);
            strcat(script, "\n");

            script += line_length;

            break;
        }
        case P8TYPE_GFX_4BIT:
        case P8TYPE_GFX_8BIT:
        case P8TYPE_GFF:
        case P8TYPE_LABEL:
        case P8TYPE_MAP:
        {
            uint8_t *write_mem = memory + m_p8_mem_offset[p8_type] + write_offset;
            hex_to_bytes(write_mem, line, &write_length);

            write_offset += write_length;
            break;
        }
        case P8TYPE_SFX:
        {
            uint8_t *read_mem = memory + MEMORY_SFX + read_offset;
            uint8_t *write_mem = memory + MEMORY_SFX + write_offset;

            hex_to_bytes(read_mem, line, &read_length);

            if (read_length > 0)
                read_sfx(write_mem, read_mem, read_length, &write_length);

            read_offset += read_length;
            write_offset += write_length;
            break;
        }
        case P8TYPE_MUSIC:
        {
            uint8_t *read_mem = memory + MEMORY_MUSIC + read_offset;
            uint8_t *write_mem = memory + MEMORY_MUSIC + write_offset;

            hex_to_bytes(read_mem, line, &read_length);

            if (read_length > 0)
                read_music(write_mem, read_mem, read_length, &write_length);

            read_offset += read_length;
            write_offset += write_length;
            break;
        }
        }
    }

    for (int i = MEMORY_SPRITES; i < MEMORY_SPRITES + MEMORY_SPRITES_SIZE; i++)
    {
        memory[i] = (memory[i] << 4) | (memory[i] >> 4);
    }

    for (int i = MEMORY_SPRITES_MAP; i < MEMORY_SPRITES_MAP + MEMORY_SPRITES_MAP_SIZE; i++)
    {
        memory[i] = (memory[i] << 4) | (memory[i] >> 4);
    }
}

void parse_p8_file(char *file_name, uint8_t *memory, char **lua_script)
{
    FILE *file = fopen(file_name, "r");

    if (file == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", file_name);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char source[file_size];

    fread(source, 1, file_size, file);

    parse_p8_ram((uint8_t *)source, (int)file_size, memory, lua_script);

    fclose(file);
}

void parse_png(char *file_name, uint8_t *memory, char **lua_script)
{
    // TODO: Implement
}
