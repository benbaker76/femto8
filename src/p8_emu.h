/*
 * p8_emu.h
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#ifndef P8_EMU_H
#define P8_EMU_H

#ifdef __DA1470x__
#define OS_FREERTOS
#else
#define SDL
#define ENABLE_AUDIO
#endif

// #define BOOL_NULL -1
#define PI 3.14159265358f
#define TWO_PI 6.28318530718f

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? a : b)
#define MIN(a, b) ((a) < (b) ? a : b)
#endif
#define IS_EVEN(n) ((n ^ 1) == (n + 1))
#define NIBBLE_SWAP(n) ((n << 4) | (n >> 4))
#define ARGB_TO_RGB565(argb) (((argb >> 8) & 0xF800) | ((argb >> 5) & 0x07E0) | ((argb >> 3) & 0x001F))

#ifdef __DA1470x__
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#else
#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 512
#endif

#define P8_WIDTH 128
#define P8_HEIGHT 128
#define MEMORY_SPRITES 0
#define MEMORY_SPRITES_SIZE 0x1000
#define MEMORY_SPRITES_MAP 0x1000
#define MEMORY_SPRITES_MAP_SIZE 0x1000
#define MEMORY_MAP 0x2000
#define MEMORY_MAP_SIZE 0x1000
#define MEMORY_SPRITEFLAGS 0x3000
#define MEMORY_SPRITEFLAGS_SIZE 0x100
#define MEMORY_MUSIC 0x3100
#define MEMORY_MUSIC_SIZE 0x100
#define MEMORY_SFX 0x3200
#define MEMORY_SFX_SIZE 0x1100
#define MEMORY_WORKRAM 0x4300
#define MEMORY_WORKRAM_SIZE 0x1b00
#define MEMORY_FONT 0x5600
#define MEMORY_FONT_SIZE 0x2000
#define MEMORY_CARTDATA 0x5e00
#define MEMORY_CARTDATA_SIZE 0x100
#define MEMORY_DRAWSTATE 0x5f00
#define MEMORY_DRAWSTATE_SIZE 0x40
#define MEMORY_HARDWARESTATE 0x5f40
#define MEMORY_HARDWARESTATE_SIZE 0x40
#define MEMORY_GPIO 0x5f80
#define MEMORY_GPIO_SIZE 0x80
#define MEMORY_SCREEN 0x6000
#define MEMORY_SCREEN_SIZE 0x2000

#define MEMORY_SIZE 1024 * 32
#define MEMORY_LUA_SIZE 1024 * 32

#define MEMORY_PALETTES 0x5f00
#define MEMORY_CLIPRECT 0x5f20
#define MEMORY_PENCOLOR 0x5f25
#define MEMORY_CURSOR 0x5f26
#define MEMORY_CAMERA 0x5f28

#define TICKS_PER_SECOND 128
#define SCREEN_SIZE 128 * 128 * 4
#define GLYPH_WIDTH 4
#define GLYPH_HEIGHT 6
#define SPRITE_WIDTH 8
#define SPRITE_HEIGHT 8
#define BUTTON_COUNT 6
#define PLAYER_COUNT 2
#define STAT_FRAMERATE 7

#define INPUT_LEFT SDLK_LEFT
#define INPUT_RIGHT SDLK_RIGHT
#define INPUT_UP SDLK_UP
#define INPUT_DOWN SDLK_DOWN
#define INPUT_ACTION1 SDLK_z
#define INPUT_ACTION2 SDLK_x

enum
{
    PALTYPE_DRAW,
    PALTYPE_SCREEN
};

enum
{
    BUTTON_LEFT = 0x0001,
    BUTTON_RIGHT = 0x0002,
    BUTTON_UP = 0x0004,
    BUTTON_DOWN = 0x0008,
    BUTTON_ACTION1 = 0x0010,
    BUTTON_ACTION2 = 0x0020,
};

extern float m_fps;
extern float m_actual_fps;
extern float m_time;

extern unsigned char *m_memory;
extern char *m_font;

extern int m_mouse_x, m_mouse_y;

extern uint8_t m_buttons[2];
extern uint8_t m_prev_buttons[2];

int p8_init_file(char *file_name);
int p8_init_ram(uint8_t *buffer, int size);
int p8_shutdown(void);
void p8_render();

#endif
