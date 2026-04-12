/*
 * p8_emu.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <direct.h>   // _mkdir
#define MKDIR(p) _mkdir(p)
#else
#include <sys/types.h>
#include <sys/stat.h> // mkdir
#define MKDIR(p) mkdir((p), 0777)
#endif
#include <unistd.h>
#include <math.h>
#include <assert.h>
#ifdef OS_FREERTOS
#include <FreeRTOS.h>
#include <task.h>
#include "retro_heap.h"
#include "ble_controller.h"
#endif
#include "p8_audio.h"
#include "p8_compat.h"
#include "p8_dialog.h"
#include "p8_emu.h"
#include "p8_lua.h"
#include "p8_lua_helper.h"
#include "p8_overlay_helper.h"
#include "p8_parser.h"
#include "p8_pause_menu.h"

#ifdef SDL
#include "SDL.h"
#else
#include "gdi.h"
#endif

#ifdef SDL
// ARGB
uint32_t m_colors[32] = {
    0x00000000, 0x001d2b53, 0x007e2553, 0x00008751, 0x00ab5236, 0x005f574f, 0x00c2c3c7, 0x00fff1e8,
    0x00ff004d, 0x00ffa300, 0x00ffec27, 0x0000e436, 0x0029adff, 0x0083769c, 0x00ff77a8, 0x00ffccaa,
    0x00291814, 0x00111D35, 0x00422136, 0x00125359, 0x00742F29, 0x0049333B, 0x00A28879, 0x00F3EF7D,
    0x00BE1250, 0x00FF6C24, 0x00A8E72E, 0x0000B54E, 0x00065AB5, 0x00754665, 0x00FF6E59, 0x00FF9D81};
#else
// RGB565
uint16_t m_colors[32] = {
    0x0000, 0x194a, 0x792a, 0x042a, 0xaa86, 0x5aa9, 0xc618, 0xff9d, 0xf809, 0xfd00, 0xff64, 0x0726, 0x2d7f, 0x83b3, 0xfbb5, 0xfe75,
    0x28c2, 0x10e6, 0x4106, 0x128b, 0x7165, 0x4987, 0xa44f, 0xf76f, 0xb88a, 0xfb64, 0xaf25, 0x05a9, 0x02d6, 0x722c, 0xfb6b, 0xfcf0};
#endif

// Convert 8-bit screen palette value to 5-bit color index.
static inline int color_index(uint8_t c)
{
    return ((c >> 3) & 0x10) | (c & 0xf);
}

static int p8_init_lcd(void);
static void p8_main_loop();
static void p8_show_compatibility_error(int severity);

uint8_t *m_memory = NULL;
uint8_t *m_cart_memory = NULL;

uint8_t *m_overlay_memory = NULL;

unsigned m_fps = 30;
unsigned m_actual_fps = 0;
unsigned m_frames = 0;

p8_clock_t m_start_time;

jmp_buf jmpbuf_restart;
static bool restart;

static jmp_buf jmpbuf_load;
bool m_load_available = false;
static bool load_requested = false;
static char *load_filename = NULL;
static char *load_param = NULL;
char *current_cart_dir = NULL;

static bool skip_compat_check = false;
static bool skip_main_loop_if_no_callbacks = false;

const char *m_param_string = "";

#ifdef SDL
SDL_Surface *m_screen = NULL;
SDL_Surface *m_output = NULL;
SDL_PixelFormat *m_format = NULL;
#else
SemaphoreHandle_t m_drawSemaphore;
#endif

int16_t m_mouse_x, m_mouse_y;
int16_t mouse_x4, mouse_y4;
int16_t m_mouse_xrel, m_mouse_yrel;
uint8_t m_mouse_buttons;
int8_t m_mouse_wheel;
uint8_t m_keypress;
bool m_scancodes[NUM_SCANCODES];

uint16_t m_buttons[PLAYER_COUNT];
uint16_t m_buttonsp[PLAYER_COUNT];
uint16_t m_button_first_repeat[PLAYER_COUNT];
unsigned m_button_down_time[PLAYER_COUNT][BUTTON_INTERNAL_COUNT];

static bool m_prev_pointer_lock;

static FILE *cartdata = NULL;
static bool cartdata_needs_flush = false;

static int m_initialized = 0;

const uint8_t io_icon[32] = {
    0x00, 0x77, 0x77, 0x77,
    0x00, 0x17, 0x11, 0x71,
    0x00, 0x17, 0x77, 0x71,
    0x00, 0x17, 0x77, 0x71,
    0x00, 0x17, 0x11, 0x71,
    0x00, 0x77, 0x77, 0x77,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

static p8_clock_t p8_clock(void)
{
#ifdef OS_FREERTOS
    return xTaskGetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * UINT64_C(1000000) + ts.tv_nsec / UINT64_C(1000);
#endif
}

static unsigned p8_clock_ms(p8_clock_t clocks)
{
#ifdef OS_FREERTOS
    return clocks * portTICK_PERIOD_MS;
#else
    return clocks / UINT64_C(1000);
#endif
}

static p8_clock_t p8_clock_delta(p8_clock_t start, p8_clock_t end)
{
    return end - start;
}

static void p8_sleep(unsigned ms)
{
#ifdef OS_FREERTOS
    vTaskDelay(pdMS_TO_TICKS(ms));
#else
    usleep(ms * 1000);
#endif
}

int p8_init()
{
    assert(!m_initialized);

    srand((unsigned int)time(NULL));

#ifdef SDL
    m_memory = (uint8_t *)malloc(MEMORY_SIZE);
    m_cart_memory = (uint8_t *)malloc(CART_MEMORY_SIZE);
    m_overlay_memory = (uint8_t *)malloc(MEMORY_SCREEN_SIZE);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        printf("Error on SDL_Init().\n");
        return 1;
    }

    SDL_ShowCursor(0);
    SDL_EnableKeyRepeat(0, 0);

    m_screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_HWSURFACE);
    m_format = m_screen->format;

    m_output = SDL_CreateRGBSurface(0, P8_WIDTH, P8_HEIGHT, 32, m_format->Rmask, m_format->Gmask, m_format->Bmask, m_format->Amask);

    SDL_WM_SetCaption("femto-8", NULL);
#else
    m_drawSemaphore = xSemaphoreCreateBinary();

    xSemaphoreGive(m_drawSemaphore);

    m_memory = (uint8_t *)rh_malloc(MEMORY_SIZE);
    m_cart_memory = (uint8_t *)rh_malloc(CART_MEMORY_SIZE);
    m_overlay_memory = (uint8_t *)rh_malloc(MEMORY_SCREEN_SIZE);
#endif

    memset(m_memory, 0, MEMORY_SIZE);
    memset(m_cart_memory, 0, CART_MEMORY_SIZE);
    memset(m_overlay_memory, (OVERLAY_TRANSPARENT_COLOR << 4) | OVERLAY_TRANSPARENT_COLOR, MEMORY_SCREEN_SIZE);

#ifdef ENABLE_AUDIO
    audio_init();
#endif

    p8_init_lcd();

    for (unsigned p=0;p<2;++p) {
        for (unsigned i=0;i<BUTTON_INTERNAL_COUNT;++i) {
            m_button_down_time[p][i] = UINT_MAX;
        }
    }

    m_initialized = 1;

    return 0;
}

static int p8_init_lcd(void)
{
#ifndef SDL
    gdi_set_layer_start(HW_LCDC_LAYER_0, 0, 0);

    gdi_set_layer_enable(HW_LCDC_LAYER_0, true);

    uint16_t *fb = (uint16_t *)gdi_get_frame_buffer_addr(HW_LCDC_LAYER_0);

    gdi_set_layer_src(HW_LCDC_LAYER_0, fb, SCREEN_WIDTH, SCREEN_HEIGHT, GDI_FORMAT_RGB565);
#endif

    return 0;
}

static int p8_init_common(const char *file_name, const char *lua_script)
{
    p8_show_io_icon(false);

    if (lua_script == NULL) {
        if (file_name) fprintf(stderr, "%s: ", file_name);
        fprintf(stderr, "invalid cart\n");
        return -1;
    }

    if (setjmp(jmpbuf_restart)) {
        if (!restart)
            return 0;
    }

    if (!skip_compat_check) {
        int ret = check_compatibility(file_name, lua_script);
        if (ret != COMPAT_OK)
            p8_show_compatibility_error(ret);
        if (ret == COMPAT_NONE)
            return -1;
    }
    restart = false;
    // Skip compat check after first cart
    skip_compat_check = true;

    memcpy(m_memory, m_cart_memory, CART_MEMORY_SIZE);

    m_frames = 0;

    p8_reset();
    clear_screen(0);

    p8_update_input();

    lua_init_script(lua_script);

    lua_init();

    if (!skip_main_loop_if_no_callbacks || lua_has_main_loop_callbacks())
        p8_main_loop();
    return 0;
}

int p8_init_file_with_param(const char *file_name, const char *param)
{
    if (!m_initialized)
        p8_init();

    m_param_string = param ? param : "";
    m_load_available = true;

    const char *lua_script = NULL;
    uint8_t *file_buffer = NULL;

    if (setjmp(jmpbuf_load)) {
        load_requested = false;

#ifdef OS_FREERTOS
        rh_free(file_buffer);
#else
        free(file_buffer);
#endif

        lua_shutdown_api();

        m_param_string = load_param ? load_param : "";
        file_name = load_filename;
    }

    if (current_cart_dir) {
#ifdef OS_FREERTOS
        rh_free(current_cart_dir);
#else
        free(current_cart_dir);
#endif
    }

    const char *last_slash = strrchr(file_name, '/');
    if (last_slash) {
        size_t dir_len = last_slash - file_name;
#ifdef OS_FREERTOS
        current_cart_dir = rh_malloc(dir_len + 1);
#else
        current_cart_dir = malloc(dir_len + 1);
#endif
        memcpy(current_cart_dir, file_name, dir_len);
        current_cart_dir[dir_len] = '\0';
    } else {
        current_cart_dir = strdup(".");
    }

    p8_show_io_icon(true);
    lua_load_api();

    printf("Loading %s\n", file_name);
    if (parse_cart_file(file_name, m_cart_memory, &lua_script, &file_buffer, NULL) != 0)
        return -1;

    int ret = p8_init_common(file_name, lua_script);

#ifdef OS_FREERTOS
    rh_free(file_buffer);
#else
    free(file_buffer);
#endif

    return ret;
}

int p8_init_ram(uint8_t *buffer, int size)
{
    if (!m_initialized)
        p8_init();

    p8_show_io_icon(true);
    lua_load_api();

    const char *lua_script = NULL;
    uint8_t *decompression_buffer = NULL;

    parse_cart_ram(buffer, size, m_cart_memory, &lua_script, &decompression_buffer);

    // printf("%s", m_lua_script);

    int init_ret = p8_init_common(NULL, lua_script);

#ifdef OS_FREERTOS
    rh_free(decompression_buffer);
#else
    free(decompression_buffer);
#endif

    return init_ret;
}

int p8_shutdown()
{
    audio_close();

    lua_shutdown_api();

    p8_close_cartdata();

#ifdef SDL
    SDL_FreeSurface(m_output);
    SDL_FreeSurface(m_screen);
    SDL_Quit();

    free(m_cart_memory);
    free(m_memory);
    free(m_overlay_memory);
#else
    rh_free(m_cart_memory);
    rh_free(m_memory);
    rh_free(m_overlay_memory);
#endif

    m_initialized = 0;

    return 0;
}

#ifdef SDL
// Map output pixel (ox, oy) to source framebuffer pixel (sx, sy) based on
// the screen transform mode at 0x5f2c.
static void screen_transform_pixel(uint8_t mode, int ox, int oy, int *sx, int *sy)
{
    switch (mode) {
    default:
    case 0: // normal
        *sx = ox; *sy = oy;
        break;
    case 1: // horizontal stretch: 64x128 → 128x128
        *sx = ox >> 1; *sy = oy;
        break;
    case 2: // vertical stretch: 128x64 → 128x128
        *sx = ox; *sy = oy >> 1;
        break;
    case 3: // both stretch: 64x64 → 128x128
        *sx = ox >> 1; *sy = oy >> 1;
        break;
    case 5: // horizontal mirror: left half mirrored to right
        *sx = (ox < 64) ? ox : (127 - ox); *sy = oy;
        break;
    case 6: // vertical mirror: top half mirrored to bottom
        *sx = ox; *sy = (oy < 64) ? oy : (127 - oy);
        break;
    case 7: // both mirror: top-left quarter mirrored to all
        *sx = (ox < 64) ? ox : (127 - ox);
        *sy = (oy < 64) ? oy : (127 - oy);
        break;
    case 129: // horizontal flip
        *sx = 127 - ox; *sy = oy;
        break;
    case 130: // vertical flip
        *sx = ox; *sy = 127 - oy;
        break;
    case 131: // both flip (180 degree rotation)
        *sx = 127 - ox; *sy = 127 - oy;
        break;
    case 133: // clockwise 90 degree rotation
        *sx = 127 - oy; *sy = ox;
        break;
    case 134: // 180 degree rotation
        *sx = 127 - ox; *sy = 127 - oy;
        break;
    case 135: // counterclockwise 90 degree rotation
        *sx = oy; *sy = 127 - ox;
        break;
    }
}

// Resolve palette for a framebuffer pixel given the high-color mode.
// pix_index is the raw 4-bit pixel value, sy is the source scanline,
// sx is the source x coordinate.
static uint8_t high_color_resolve(uint8_t hc_mode, uint8_t pix_index, int sx, int sy)
{
    if (hc_mode == 0x10) {
        // Per-line palette swap: use secondary palette if bit set in bitfield
        uint8_t bf = m_memory[0x5f70 + (sy >> 3)];
        if (bf & (1 << (sy & 7)))
            return color_get(PALTYPE_SECONDARY, pix_index);
        return color_get(PALTYPE_SCREEN, pix_index);
    }
    if (hc_mode == 0x20) {
        // 5-bitplane mode: requires 0x5f2c == 1 (horizontal stretch).
        // If the corresponding pixel in the hidden right half (sx+64) is
        // non-zero, use secondary palette.
        int hidden_offset = (m_memory[MEMORY_SCREEN_PHYS] << 8) + ((sx + 64) >> 1) + sy * 64;
        uint8_t hidden_val = m_memory[hidden_offset];
        uint8_t hidden_pix = IS_EVEN(sx + 64) ? (hidden_val & 0xF) : (hidden_val >> 4);
        if (hidden_pix != 0)
            return color_get(PALTYPE_SECONDARY, pix_index);
        return color_get(PALTYPE_SCREEN, pix_index);
    }
    if ((hc_mode & 0xf0) == 0x30) {
        // Gradient fill: replace color n with per-section gradient
        uint8_t replace_color = hc_mode & 0x0f;
        uint8_t screen_index = color_get(PALTYPE_SCREEN, pix_index);
        if ((screen_index & 0x0f) == replace_color) {
            int section = sy >> 3;
            uint8_t bf = m_memory[0x5f70 + (sy >> 3)];
            if (bf & (1 << (sy & 7)))
                section = (section + 1) & 0x0f;
            return color_get(PALTYPE_SECONDARY, section);
        }
        return screen_index;
    }
    return color_get(PALTYPE_SCREEN, pix_index);
}

void p8_render()
{
    sprintf(m_str_buffer, "%d", (int)m_actual_fps);
    draw_simple_text(m_str_buffer, 0, 0, 1);

    uint32_t *output = m_output->pixels;
    uint8_t transform = m_memory[MEMORY_SCREEN_TRANSFORM];
    uint8_t hc_mode = m_memory[MEMORY_HIGH_COLOUR_MODE];

    for (int y = 0; y < P8_HEIGHT; y++)
    {
        for (int x = 0; x < P8_WIDTH; x++)
        {
            int sx, sy;
            screen_transform_pixel(transform, x, y, &sx, &sy);
            int screen_offset = (m_memory[MEMORY_SCREEN_PHYS] << 8) + (sx >> 1) + sy * 64;
            uint8_t value = m_memory[screen_offset];
            uint8_t pix = IS_EVEN(sx) ? value & 0xF : value >> 4;
            uint8_t index;
            if (hc_mode != 0)
                index = high_color_resolve(hc_mode, pix, sx, sy);
            else
                index = color_get(PALTYPE_SCREEN, pix);
            uint32_t color = m_colors[color_index(index)];

            output[x + (y * P8_WIDTH)] = color;
        }
    }

    uint8_t *overlay_mem = m_overlay_memory;

    for (int y = 0; y < P8_HEIGHT; y++)
    {
        for (int x = 0; x < P8_WIDTH; x++)
        {
            int overlay_offset = (x >> 1) + y * 64;
            uint8_t value = overlay_mem[overlay_offset];
            uint8_t pixel_color = IS_EVEN(x) ? (value & 0xF) : (value >> 4);

            if (pixel_color != OVERLAY_TRANSPARENT_COLOR)
            {
                uint32_t color = m_colors[color_index(pixel_color)];
                output[x + (y * P8_WIDTH)] = color;
            }
        }
    }

    SDL_Rect rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    // SDL_BlitSurface(m_output, NULL, m_screen, &rect);
    SDL_SoftStretch(m_output, NULL, m_screen, &rect);
    SDL_Flip(m_screen);
}
#else

void draw_complete(bool underflow, void *user_data)
{
    xSemaphoreGive(m_drawSemaphore);
}

void p8_render()
{
    if (xSemaphoreTake(m_drawSemaphore, portMAX_DELAY) != pdTRUE)
        return;

    sprintf(m_str_buffer, "%d", (int)m_actual_fps);
    draw_text(m_str_buffer, 0, 0, 1);

    uint16_t *output = gdi_get_frame_buffer_addr(HW_LCDC_LAYER_0);
    uint8_t *screen_mem = &m_memory[(m_memory[MEMORY_SCREEN_PHYS] << 8)];
    uint8_t *pal = &m_memory[MEMORY_PALETTES + PALTYPE_SCREEN * 16];

    for (int y = 1; y <= 128; y++)
    {
        if (y & 0x7)
        {
            uint16_t *top = output;
            uint16_t *bottom = output + 240;

            for (int x = 0; x < 128; x += 8)
            {

                uint8_t left = (*screen_mem) & 0xF;
                uint8_t right = (*screen_mem) >> 4;

                uint8_t index_left = pal[left];
                uint8_t index_right = color_pal[right];

                uint16_t c_left = m_colors[color_index(index_left)];
                uint16_t c_right = m_colors[color_index(index_right)];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                *bottom++ = c_left;
                *bottom++ = c_left;
                *bottom++ = c_right;
                *bottom++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left];
                index_right = pal[right];

                c_left = m_colors[color_index(index_left)];
                c_right = m_colors[color_index(index_right)];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                *bottom++ = c_left;
                *bottom++ = c_left;
                *bottom++ = c_right;
                *bottom++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left];
                index_right = pal[right];

                c_left = m_colors[color_index(index_left)];
                c_right = m_colors[color_index(index_right)];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                *bottom++ = c_left;
                *bottom++ = c_left;
                *bottom++ = c_right;
                *bottom++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left];
                index_right = pal[right];

                c_left = m_colors[color_index(index_left)];
                c_right = m_colors[color_index(index_right)];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;

                *bottom++ = c_left;
                *bottom++ = c_right;
                *bottom++ = c_right;

                screen_mem++;
            }

            output += 480;
        }
        else
        {
            uint16_t *top = output;

            for (int x = 0; x < 128; x += 8)
            {

                uint8_t left = (*screen_mem) & 0xF;
                uint8_t right = (*screen_mem) >> 4;

                uint8_t index_left = pal[left];
                uint8_t index_right = pal[right];

                uint16_t c_left = m_colors[color_index(index_left)];
                uint16_t c_right = m_colors[color_index(index_right)];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left];
                index_right = pal[right];

                c_left = m_colors[color_index(index_left)];
                c_right = m_colors[color_index(index_right)];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left];
                index_right = pal[right];

                c_left = m_colors[color_index(index_left)];
                c_right = m_colors[color_index(index_right)];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left];
                index_right = pal[right];

                c_left = m_colors[color_index(index_left)];
                c_right = m_colors[color_index(index_right)];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;

                screen_mem++;
            }

            output += 240;
        }
    }

    output = gdi_get_frame_buffer_addr(HW_LCDC_LAYER_0);

    for (int y = 1; y <= P8_HEIGHT; y++)
    {
        if (y & 0x7)
        {
            uint16_t *top = output;

            for (int x = 0; x < 128; x += 2)
            {
                int overlay_offset = (x >> 1) + (y - 1) * 64;
                uint8_t value = m_overlay_memory[overlay_offset];
                uint8_t left = value & 0xF;
                uint8_t right = value >> 4;

                if (left != OVERLAY_TRANSPARENT_COLOR)
                {
                    uint16_t c_left = m_colors[color_index(left)];
                    *top = c_left;
                    *(top + 1) = c_left;
                }
                top += 2;

                if (right != OVERLAY_TRANSPARENT_COLOR)
                {
                    uint16_t c_right = m_colors[color_index(right)];
                    *top = c_right;
                    *(top + 1) = c_right;
                }
                top += 2;
            }

            output += 240;
        }
    }

    gdi_display_update_async(draw_complete, NULL);
}
#endif

void p8_update_input()
{
    bool pointer_lock = (m_memory[MEMORY_DEVKIT_MODE] & 0x4) != 0;
    if (pointer_lock != m_prev_pointer_lock) {
        m_prev_pointer_lock  = pointer_lock;
#ifdef SDL
        SDL_WM_GrabInput(pointer_lock ? SDL_GRAB_ON : SDL_GRAB_OFF);
#endif
    }

#ifdef SDL
    m_mouse_xrel = 0;
    m_mouse_yrel = 0;
    m_mouse_wheel = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_MOUSEMOTION:
            m_mouse_x = event.motion.x * P8_WIDTH / SCREEN_WIDTH;
            m_mouse_y = event.motion.y * P8_HEIGHT / SCREEN_HEIGHT;
            m_mouse_xrel += event.motion.xrel * P8_WIDTH / SCREEN_WIDTH;
            m_mouse_yrel += event.motion.yrel * P8_HEIGHT / SCREEN_HEIGHT;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == 1) {
                m_mouse_buttons |= 0x1;
                if (m_memory[MEMORY_DEVKIT_MODE] & 0x2)
                    update_buttons(0, BUTTON_ACTION1, true);
            } else if (event.button.button == 3) {
                m_mouse_buttons |= 0x2;
                if (m_memory[MEMORY_DEVKIT_MODE] & 0x2)
                    update_buttons(0, BUTTON_ACTION2, true);
            } else if (event.button.button == 2) {
                m_mouse_buttons |= 0x4;
                if (m_memory[MEMORY_DEVKIT_MODE] & 0x2)
                    update_buttons(0, BUTTON_PAUSE, true);
            } else if (event.button.button == 4) {
                m_mouse_wheel += 1;
            } else if (event.button.button == 5) {
                m_mouse_wheel -= 1;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == 1) {
                m_mouse_buttons &= ~0x1;
                if (m_memory[MEMORY_DEVKIT_MODE] & 0x2)
                    update_buttons(0, BUTTON_ACTION1, false);
            } else if (event.button.button == 3) {
                m_mouse_buttons &= ~0x2;
                if (m_memory[MEMORY_DEVKIT_MODE] & 0x2)
                    update_buttons(0, BUTTON_ACTION2, false);
            } else if (event.button.button == 2) {
                m_mouse_buttons &= ~0x4;
                if (m_memory[MEMORY_DEVKIT_MODE] & 0x2)
                    update_buttons(0, BUTTON_PAUSE, false);
            }
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case INPUT_LEFT:
                update_buttons(0, BUTTON_LEFT, true);
                break;
            case INPUT_RIGHT:
                update_buttons(0, BUTTON_RIGHT, true);
                break;
            case INPUT_UP:
                update_buttons(0, BUTTON_UP, true);
                break;
            case INPUT_DOWN:
                update_buttons(0, BUTTON_DOWN, true);
                break;
            case INPUT_ACTION1:
                update_buttons(0, BUTTON_ACTION1, true);
                break;
            case INPUT_ACTION2:
                update_buttons(0, BUTTON_ACTION2, true);
                break;
            case INPUT_ESCAPE:
                update_buttons(0, BUTTON_ESCAPE, true);
                break;
            case SDLK_RETURN:
                update_buttons(0, BUTTON_PAUSE, true);
                update_buttons(0, BUTTON_RETURN, true);
                break;
            case SDLK_p:
                update_buttons(0, BUTTON_PAUSE, true);
                break;
            case SDLK_SPACE:
                update_buttons(0, BUTTON_SPACE, true);
                break;
            case SDLK_PAGEUP:
                update_buttons(0, BUTTON_PAGE_UP, true);
                break;
            case SDLK_PAGEDOWN:
                update_buttons(0, BUTTON_PAGE_DOWN, true);
                break;
            default:
                break;
            }
            if (event.key.keysym.scancode < NUM_SCANCODES)
                m_scancodes[event.key.keysym.scancode] = true;
            m_keypress = (event.key.keysym.sym < 256) ? event.key.keysym.sym : 0;
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
            case INPUT_LEFT:
                update_buttons(0, BUTTON_LEFT, false);
                break;
            case INPUT_RIGHT:
                update_buttons(0, BUTTON_RIGHT, false);
                break;
            case INPUT_UP:
                update_buttons(0, BUTTON_UP, false);
                break;
            case INPUT_DOWN:
                update_buttons(0, BUTTON_DOWN, false);
                break;
            case INPUT_ACTION1:
                update_buttons(0, BUTTON_ACTION1, false);
                break;
            case INPUT_ACTION2:
                update_buttons(0, BUTTON_ACTION2, false);
                break;
            case SDLK_RETURN:
                update_buttons(0, BUTTON_PAUSE, false);
                update_buttons(0, BUTTON_RETURN, false);
                break;
            case SDLK_p:
                update_buttons(0, BUTTON_PAUSE, false);
                break;
            case SDLK_SPACE:
                update_buttons(0, BUTTON_SPACE, false);
                break;
            case SDLK_PAGEUP:
                update_buttons(0, BUTTON_PAGE_UP, false);
                break;
            case SDLK_PAGEDOWN:
                update_buttons(0, BUTTON_PAGE_DOWN, false);
                break;
            case INPUT_ESCAPE:
                update_buttons(0, BUTTON_ESCAPE, false);
                break;
            default:
                break;
            }
            if (event.key.keysym.scancode < NUM_SCANCODES)
                m_scancodes[event.key.keysym.scancode] = false;
            break;
        case SDL_QUIT:
            p8_abort();
            break;
        default:
            break;
        }
    }
#else
    uint16_t mask = 0;

    if (gamepad & AXIS_L_LEFT)
        mask |= BUTTON_MASK_LEFT;
    if (gamepad & AXIS_L_RIGHT)
        mask |= BUTTON_MASK_RIGHT;
    if (gamepad & AXIS_L_UP)
        mask |= BUTTON_MASK_UP;
    if (gamepad & AXIS_L_DOWN)
        mask |= BUTTON_MASK_DOWN;
    if (gamepad & AXIS_L_TRIGGER)
        mask |= BUTTON_MASK_ACTION1;
    if (gamepad & AXIS_R_LEFT)
        mask |= BUTTON_MASK_LEFT;
    if (gamepad & AXIS_R_RIGHT)
        mask |= BUTTON_MASK_RIGHT;
    if (gamepad & AXIS_R_UP)
        mask |= BUTTON_MASK_UP;
    if (gamepad & AXIS_R_DOWN)
        mask |= BUTTON_MASK_DOWN;
    if (gamepad & AXIS_R_TRIGGER)
        mask |= BUTTON_MASK_ACTION2;
    if (gamepad & DPAD_UP)
        mask |= BUTTON_MASK_UP;
    if (gamepad & DPAD_RIGHT)
        mask |= BUTTON_MASK_RIGHT;
    if (gamepad & DPAD_DOWN)
        mask |= BUTTON_MASK_DOWN;
    if (gamepad & DPAD_LEFT)
        mask |= BUTTON_MASK_LEFT;
    if (gamepad & BUTTON_1)
        mask |= BUTTON_MASK_ACTION1;
    if (gamepad & BUTTON_2)
        mask |= BUTTON_MASK_ACTION2;

    m_buttons[0] = mask;

#endif

    if (m_memory[MEMORY_DEVKIT_MODE] & 0x2)
        m_buttons[0] |= (((m_mouse_buttons >> 0) & 1) << 4) | (((m_mouse_buttons >> 1) & 1) << 5) | (((m_mouse_buttons >> 2) & 1) << 6);

    uint8_t delay = m_memory[MEMORY_AUTO_REPEAT_DELAY];
    if (delay == 0)
        delay = DEFAULT_AUTO_REPEAT_DELAY;
    uint8_t interval = m_memory[MEMORY_AUTO_REPEAT_INTERVAL];
    if (interval == 0)
        interval = DEFAULT_AUTO_REPEAT_INTERVAL;
    for (unsigned p=0;p<PLAYER_COUNT;++p) {
        m_buttonsp[p] = 0;
        for (unsigned i=0;i<BUTTON_INTERNAL_COUNT;++i) {
            if (m_buttons[p] & (1 << i)) {
                if (m_button_down_time[p][i] == UINT_MAX) {
                    // ignore buttons pressed at startup
                } else if (!m_button_down_time[p][i]) {
                    m_button_down_time[p][i] = m_frames;
                    m_buttonsp[p] |= 1 << i;
                } else if (i < BUTTON_REPEAT_COUNT) {
                    if (delay != 255 && !(m_button_first_repeat[p] & (1 << i)) && m_frames - m_button_down_time[p][i] >= delay) {
                        m_button_down_time[p][i] = m_frames;
                        m_button_first_repeat[p] |= 1 << i;
                        m_buttonsp[p] |= 1 << i;
                    } else if ((m_button_first_repeat[p] & (1 << i)) && m_frames - m_button_down_time[p][i] >= interval) {
                        m_button_down_time[p][i] = m_frames;
                        m_buttonsp[p] |= 1 << i;
                    }
                }
            } else  {
                if (m_button_down_time[p][i]) {
                    m_button_down_time[p][i] = 0;
                    m_button_first_repeat[p] &= ~(1 << i);
                }
            }
        }
    }

    if (!m_dialog_showing) {
        if ((m_buttons[0] & BUTTON_MASK_ESCAPE) != 0)
            p8_abort();

        if ((m_buttonsp[0] & BUTTON_MASK_PAUSE) != 0) {
            p8_show_pause_menu();
        }
    }
}

static void p8_post_flip(void)
{
    p8_flush_cartdata();
    p8_update_input();
    m_frames++;
}

void p8_flip()
{
    p8_render();

    unsigned elapsed_time = p8_elapsed_time();
    const unsigned target_frame_time = 1000 / m_fps;
    int sleep_time = target_frame_time - elapsed_time;
    if (sleep_time < 0)
        sleep_time = 0;
    m_actual_fps = 1000 / (elapsed_time + sleep_time);

    if (sleep_time > 0)
        p8_sleep(sleep_time);

    m_start_time = p8_clock();

    p8_post_flip();
}

static void p8_main_loop()
{
    const int target_frame_time = 1000 / m_fps;
    int time_debt = 0;
    unsigned updates_since_last_flip = 0;

    for (;;)
    {
        lua_update();
        updates_since_last_flip++;

        unsigned elapsed = p8_elapsed_time();

        time_debt += elapsed;

        if (time_debt < target_frame_time || updates_since_last_flip >= m_fps) {
            lua_draw();
            time_debt += p8_elapsed_time() - elapsed;

            p8_flip();

            if (updates_since_last_flip >= m_fps) {
                time_debt = 0;
            } else {
                time_debt -= target_frame_time;
                if (time_debt < -target_frame_time) time_debt = -target_frame_time;
            }

            updates_since_last_flip = 0;
        } else {
            p8_post_flip();

            time_debt -= target_frame_time;
        }
    }
}

unsigned p8_elapsed_time(void)
{
    p8_clock_t now = p8_clock();
    unsigned elapsed_time;
    if (m_start_time == 0)
        elapsed_time = 0;
    else
        elapsed_time = p8_clock_ms(p8_clock_delta(m_start_time, now));
    return elapsed_time;
}

void p8_pump_events(void)
{
#ifdef SDL
    SDL_PumpEvents();

    SDL_Event event;
    if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_QUITMASK | SDL_KEYDOWNMASK) > 0) {
        if (event.type == SDL_QUIT) {
            SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_QUITMASK);
            p8_abort();
        } else if (event.type == SDL_KEYDOWN) {
            if ((event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_p) &&
                (m_buttons[0] & BUTTON_MASK_PAUSE) == 0) {
                SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWNMASK);
                if (event.key.keysym.sym == SDLK_RETURN) {
                    m_buttons[0] |= BUTTON_MASK_PAUSE | BUTTON_MASK_RETURN;
                    m_button_down_time[0][BUTTON_PAUSE] = UINT_MAX;
                    m_button_down_time[0][BUTTON_RETURN] = UINT_MAX;
                } else {
                    m_buttons[0] |= BUTTON_MASK_PAUSE;
                    m_button_down_time[0][BUTTON_PAUSE] = UINT_MAX;
                }
                p8_show_pause_menu();
            } else if (event.key.keysym.sym == INPUT_ESCAPE && (m_buttons[0] & BUTTON_MASK_ESCAPE) == 0) {
                SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWNMASK);
                m_buttons[0] |= BUTTON_MASK_ESCAPE;
                m_button_down_time[0][BUTTON_ESCAPE] = UINT_MAX;
                p8_abort();
            }
        }
    }
#endif
}

void p8_seed_rng_state(uint32_t seed)
{
    uint32_t hi, lo;

    if (seed == 0) {
        hi = 0x60009755;
        lo = 0xdeadbeef;
    } else {
        seed &= 0x7fffffff;

        uint32_t seed_fixed = seed << 16;
        hi = seed_fixed ^ 0xbead29ba;
        lo = seed_fixed;
    }

    for (int i = 0; i < 32; i++) {
        hi = (hi << 16) | (hi >> 16);
        hi += lo;
        lo += hi;
    }

    m_memory[MEMORY_RNG_STATE] = hi & 0xFF;
    m_memory[MEMORY_RNG_STATE + 1] = (hi >> 8) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 2] = (hi >> 16) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 3] = (hi >> 24) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 4] = lo & 0xFF;
    m_memory[MEMORY_RNG_STATE + 5] = (lo >> 8) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 6] = (lo >> 16) & 0xFF;
    m_memory[MEMORY_RNG_STATE + 7] = (lo >> 24) & 0xFF;
}

void p8_reset(void)
{
    memset(m_memory + MEMORY_DRAWSTATE, 0, MEMORY_DRAWSTATE_SIZE);
    memset(m_memory + MEMORY_HARDWARESTATE, 0, MEMORY_HARDWARESTATE_SIZE);
    m_memory[MEMORY_SCREEN_PHYS] = 0x60;
    m_memory[MEMORY_MAP_START] = 0x20;
    m_memory[MEMORY_MAP_WIDTH] = 128;
    m_memory[MEMORY_RW_MASK] = 0xff;
    pencolor_set(6);
    reset_color();
    clip_set(0, 0, P8_WIDTH, P8_HEIGHT);
    p8_seed_rng_state(time(NULL));
}

void __attribute__ ((noreturn)) p8_abort()
{
    longjmp(jmpbuf_restart, 1);
}

void __attribute__ ((noreturn)) p8_restart()
{
    restart = true;
    p8_abort();
}

char *p8_resolve_relative_path(const char *filename)
{
    if (filename[0] == '/' || filename[1] == ':')
        return strdup(filename);

    if (!current_cart_dir)
        return strdup(filename);

    size_t len = strlen(current_cart_dir) + strlen(filename) + 2;
    char *resolved_path = malloc(len);
    if (resolved_path)
        snprintf(resolved_path, len, "%s/%s", current_cart_dir, filename);
    return resolved_path;
}

void __attribute__ ((noreturn)) p8_load_new(const char *filename, const char *param)
{
    assert(m_load_available);

    if (load_filename) {
#ifdef OS_FREERTOS
        rh_free(load_filename);
#else
        free(load_filename);
#endif
    }
    if (load_param) {
#ifdef OS_FREERTOS
        rh_free(load_param);
#else
        free(load_param);
#endif
    }

    load_filename = strdup(filename);
    load_param = param ? strdup(param) : NULL;
    load_requested = true;

    longjmp(jmpbuf_load, 1);
}

void p8_set_skip_compat_check(bool skip)
{
    skip_compat_check = skip;
}

void p8_set_skip_main_loop_if_no_callbacks(bool skip)
{
    skip_main_loop_if_no_callbacks = skip;
}

bool p8_open_cartdata(const char *id)
{
    if (cartdata)
        return false;
    int ret = MKDIR(CARTDATA_PATH);
    if (ret == -1 && errno != EEXIST) {
        return false;
    }
    char *path = alloca(strlen(CARTDATA_PATH) + 1 + strlen(id) + 1);
    sprintf(path, "%s/%s", CARTDATA_PATH, id);
    cartdata = fopen(path, "r+b");
    if (!cartdata) {
        cartdata = fopen(path, "w+b");
        if (!cartdata) {
            return false;
        }
    }
    fseek(cartdata, 0, SEEK_SET);
    uint8_t *dst = m_memory + MEMORY_CARTDATA;
    size_t n = fread(dst, 1, 0x100, cartdata);
    if (n < 0x100) {
        memset(dst + n, 0, 0x100 - n);
    }
    return true;
}

void p8_flush_cartdata(void)
{
    if (cartdata && cartdata_needs_flush) {
        cartdata_needs_flush = false;
        fseek(cartdata, 0, SEEK_SET);
        fwrite(m_memory + MEMORY_CARTDATA, 0x100, 1, cartdata);
        fflush(cartdata);
    }
}

void p8_delayed_flush_cartdata(void)
{
    cartdata_needs_flush = true;
}

void p8_close_cartdata(void)
{
    if (cartdata) {
        p8_flush_cartdata();
        fclose(cartdata);
        cartdata = NULL;
    }
}

static void p8_show_compatibility_error(int severity)
{
    m_dialog_showing = true;
    p8_reset();


    p8_dialog_control_t compat_controls_some[] = {
        DIALOG_LABEL("this cart may not be"),
        DIALOG_LABEL("fully compatible with"),
        DIALOG_LABEL(PROGNAME),
        DIALOG_SPACING(),
        DIALOG_BUTTONBAR_OK_ONLY(),
    };

    p8_dialog_control_t compat_controls_none[] = {
        DIALOG_LABEL("this cart is not"),
        DIALOG_LABEL("compatible with"),
        DIALOG_LABEL(PROGNAME),
        DIALOG_SPACING(),
        DIALOG_BUTTONBAR_OK_ONLY(),
    };

    p8_dialog_t compat_dialog;
    if (severity <= COMPAT_SOME)
        p8_dialog_init(&compat_dialog, NULL, compat_controls_some, 5, 0);
    else
        p8_dialog_init(&compat_dialog, NULL, compat_controls_none, 5, 0);

    p8_dialog_run(&compat_dialog);
    p8_dialog_cleanup(&compat_dialog);

    m_button_down_time[0][BUTTON_ACTION1] = UINT_MAX;
    m_dialog_showing = false;
}

void p8_show_io_icon(bool show)
{
    if (show) {
        overlay_draw_icon(io_icon, P8_WIDTH - 8, 0);
    } else {
        overlay_draw_rectfill(P8_WIDTH - 8, 0, P8_WIDTH-1, 7, OVERLAY_TRANSPARENT_COLOR);
        p8_dialog_draw_stack();
    }
    p8_flip();
}

void p8_show_error_dialog(const char **lines, int line_count, p8_error_severity_t severity)
{
    assert(line_count <= 4);

    int control_count = line_count + 2;
    p8_dialog_control_t controls[6];

    /* Add label controls for each line */
    for (int i = 0; i < line_count; i++) {
        controls[i] = (p8_dialog_control_t)DIALOG_LABEL(lines[i]);
    }

    /* Add spacing and button bar */
    controls[line_count] = (p8_dialog_control_t)DIALOG_SPACING();
    controls[line_count + 1] = (p8_dialog_control_t)DIALOG_BUTTONBAR_OK_ONLY();

    p8_dialog_t error_dialog;
    const char *title = (severity == P8_ERROR_WARNING) ? "warning" : "error";
    p8_dialog_init(&error_dialog, title, controls, control_count, 120);
    p8_dialog_run(&error_dialog);
    p8_dialog_cleanup(&error_dialog);
}

void p8_show_version_dialog(void)
{
    extern const char *femto8_version;
    char femto8_version_string[50];
    sprintf(femto8_version_string, "femto8 %s", femto8_version);

    p8_dialog_control_t controls[] = {
        DIALOG_LABEL(femto8_version_string),
        DIALOG_SPACING(),
        DIALOG_BUTTONBAR_OK_ONLY()
    };

    p8_dialog_t dialog;
    p8_dialog_init(&dialog, "femto8 version", controls, sizeof(controls) / sizeof(controls[0]), 0);
    p8_dialog_run(&dialog);
    p8_dialog_cleanup(&dialog);
}

