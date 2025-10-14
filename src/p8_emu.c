/*
 * p8_emu.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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
#include "p8_emu.h"
#include "p8_lua.h"
#include "p8_lua_helper.h"
#include "p8_parser.h"

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

static void p8_main_loop();
static int done = 0;

uint8_t *m_memory = NULL;

unsigned m_fps = 30;
unsigned m_actual_fps = 0;
unsigned m_frames = 0;

clock_t m_start_time;

#ifdef SDL
SDL_Surface *m_screen = NULL;
SDL_Surface *m_output = NULL;
SDL_PixelFormat *m_format = NULL;
#else
SemaphoreHandle_t m_drawSemaphore;
#endif

int m_mouse_x, m_mouse_y;

uint8_t m_buttons[2] = {0};
uint8_t m_prev_buttons[2] = {0};

int p8_init()
{
    srand((unsigned int)time(NULL));

#ifdef SDL
    m_memory = (uint8_t *)malloc(MEMORY_SIZE);
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
#endif

    memset(m_memory, 0, MEMORY_SIZE);

#ifdef ENABLE_AUDIO
    audio_init();
#endif

    return 0;
}

int p8_init_lcd()
{
#ifndef SDL
    gdi_set_layer_start(HW_LCDC_LAYER_0, 0, 0);

    gdi_set_layer_enable(HW_LCDC_LAYER_0, true);

    uint16_t *fb = (uint16_t *)gdi_get_frame_buffer_addr(HW_LCDC_LAYER_0);

    gdi_set_layer_src(HW_LCDC_LAYER_0, fb, SCREEN_WIDTH, SCREEN_HEIGHT, GDI_FORMAT_RGB565);
#endif

    return 0;
}

static void p8_init_common(const char *file_name, const char *lua_script)
{
    if (lua_script == NULL) {
        if (file_name) fprintf(stderr, "%s: ", file_name);
        fprintf(stderr, "invalid cart\n");
        exit(1);
    }

    lua_load_api();

    p8_reset();

    lua_init_script(lua_script);

    clear_screen(0);

    lua_init();

    p8_init_lcd();

    p8_main_loop();
}

int p8_init_file(char *file_name)
{
    p8_init();

    const char *lua_script = NULL;
    uint8_t *file_buffer = NULL;

    parse_cart_file(file_name, m_memory, &lua_script, &file_buffer);

    p8_init_common(file_name, lua_script);

#ifdef OS_FREERTOS
    rh_free(file_buffer);
#else
    free(file_buffer);
#endif

    return 0;
}

int p8_init_ram(uint8_t *buffer, int size)
{
    p8_init();

    const char *lua_script = NULL;
    uint8_t *decompression_buffer = NULL;

    parse_cart_ram(buffer, size, m_memory, &lua_script, &decompression_buffer);

    // printf("%s", m_lua_script);

    p8_init_common(NULL, lua_script);

#ifdef OS_FREERTOS
    rh_free(decompression_buffer);
#else
    free(decompression_buffer);
#endif

    return 0;
}

int p8_shutdown()
{
    audio_close();

    lua_shutdown_api();

#ifdef SDL
    SDL_FreeSurface(m_output);
    SDL_FreeSurface(m_screen);
    SDL_Quit();

    free(m_memory);
#else
    rh_free(m_memory);
#endif

    return 0;
}

#ifdef SDL
void p8_render()
{
    sprintf(m_str_buffer, "%d", (int)m_actual_fps);
    draw_text(m_str_buffer, 0, 0, 1);

    uint32_t *output = m_output->pixels;

    for (int y = 0; y < P8_HEIGHT; y++)
    {
        for (int x = 0; x < P8_WIDTH; x++)
        {
            int screen_offset = MEMORY_SCREEN + (x >> 1) + y * 64;
            uint8_t value = m_memory[screen_offset];
            uint8_t index = color_get(PALTYPE_SCREEN, IS_EVEN(x) ? value & 0xF : value >> 4);
            uint32_t color = m_colors[color_index(index)];

            output[x + (y * P8_WIDTH)] = color;
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
    uint8_t *screen_mem = &m_memory[MEMORY_SCREEN];
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

    gdi_display_update_async(draw_complete, NULL);
}
#endif

void p8_update_input()
{
#ifdef SDL
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_MOUSEMOTION:
            break;
        case SDL_MOUSEBUTTONDOWN:
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case INPUT_LEFT:
                update_buttons(0, 0, true);
                break;
            case INPUT_RIGHT:
                update_buttons(0, 1, true);
                break;
            case INPUT_UP:
                update_buttons(0, 2, true);
                break;
            case INPUT_DOWN:
                update_buttons(0, 3, true);
                break;
            case INPUT_ACTION1:
                update_buttons(0, 4, true);
                break;
            case INPUT_ACTION2:
                update_buttons(0, 5, true);
                break;
            default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
            case INPUT_LEFT:
                update_buttons(0, 0, false);
                break;
            case INPUT_RIGHT:
                update_buttons(0, 1, false);
                break;
            case INPUT_UP:
                update_buttons(0, 2, false);
                break;
            case INPUT_DOWN:
                update_buttons(0, 3, false);
                break;
            case INPUT_ACTION1:
                update_buttons(0, 4, false);
                break;
            case INPUT_ACTION2:
                update_buttons(0, 5, false);
                break;
            default:
                break;
            }
            break;
        case SDL_QUIT:
            done = 1;
            break;
        default:
            break;
        }
    }
#else
    uint8_t mask = 0;

    if (gamepad & AXIS_L_LEFT)
        mask |= BUTTON_LEFT;
    if (gamepad & AXIS_L_RIGHT)
        mask |= BUTTON_RIGHT;
    if (gamepad & AXIS_L_UP)
        mask |= BUTTON_UP;
    if (gamepad & AXIS_L_DOWN)
        mask |= BUTTON_DOWN;
    if (gamepad & AXIS_L_TRIGGER)
        mask |= BUTTON_ACTION1;
    if (gamepad & AXIS_R_LEFT)
        mask |= BUTTON_LEFT;
    if (gamepad & AXIS_R_RIGHT)
        mask |= BUTTON_RIGHT;
    if (gamepad & AXIS_R_UP)
        mask |= BUTTON_UP;
    if (gamepad & AXIS_R_DOWN)
        mask |= BUTTON_DOWN;
    if (gamepad & AXIS_R_TRIGGER)
        mask |= BUTTON_ACTION2;
    if (gamepad & DPAD_UP)
        mask |= BUTTON_UP;
    if (gamepad & DPAD_RIGHT)
        mask |= BUTTON_RIGHT;
    if (gamepad & DPAD_DOWN)
        mask |= BUTTON_DOWN;
    if (gamepad & DPAD_LEFT)
        mask |= BUTTON_LEFT;
    if (gamepad & BUTTON_1)
        mask |= BUTTON_ACTION1;
    if (gamepad & BUTTON_2)
        mask |= BUTTON_ACTION2;

    m_buttons[0] = mask;

#endif
}

void p8_flip()
{
    p8_update_input();

    p8_render();

    unsigned elapsed_time = p8_elapsed_time();
    const unsigned target_frame_time = 1000 / m_fps;
    int sleep_time = target_frame_time - elapsed_time;
    if (sleep_time < 0)
        sleep_time = 0;
    m_actual_fps = 1000 / (elapsed_time + sleep_time);
    m_frames++;

    if (sleep_time > 0)
    {
#ifdef OS_FREERTOS
        vTaskDelay(pdMS_TO_TICKS(sleep_time));
#else
        usleep(sleep_time * 1000);
#endif
    }

#ifdef OS_FREERTOS
    m_start_time = xTaskGetTickCount();
#else
    m_start_time = clock();
#endif
}

static void p8_main_loop()
{
    while (!done)
    {
        lua_update();
        lua_draw();

        p8_flip();
    }
}

unsigned p8_elapsed_time(void)
{
    unsigned elapsed_time;
#ifdef OS_FREERTOS
    long now = xTaskGetTickCount();
    if (start_time == 0)
        elapsed_time = 0;
    else
        elapsed_time = (now - m_start_time) * portTICK_PERIOD_MS;
#else
    clock_t now = clock();
    if (m_start_time == 0)
        elapsed_time = 0;
    else
        elapsed_time = ((now - m_start_time) + ((now < m_start_time) ? CLOCKS_PER_CLOCK_T : 0)) * (clock_t)1000 / CLOCKS_PER_SEC;
#endif
    return elapsed_time;
}

void p8_reset(void)
{
    memset(m_memory + MEMORY_DRAWSTATE, 0, MEMORY_DRAWSTATE_SIZE);
    memset(m_memory + MEMORY_HARDWARESTATE, 0, MEMORY_HARDWARESTATE_SIZE);
    pencolor_set(6);
    reset_color();
    clip_set(0, 0, P8_WIDTH, P8_HEIGHT);
}
