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
#include <time.h>
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
uint32_t m_colors[16] = {
    0x00000000, 0x001d2b53, 0x007e2553, 0x00008751, 0x00ab5236, 0x005f574f, 0x00c2c3c7, 0x00fff1e8,
    0x00ff004d, 0x00ffa300, 0x00ffec27, 0x0000e436, 0x0029adff, 0x0083769c, 0x00ff77a8, 0x00ffccaa};
#else
// RGB565
uint16_t m_colors[16] = {
    0x0000, 0x194a, 0x792a, 0x042a, 0xaa86, 0x5aa9, 0xc618, 0xff9d, 0xf809, 0xfd00, 0xff64, 0x0726, 0x2d7f, 0x83b3, 0xfbb5, 0xfe75};
#endif

void p8_main_loop();

uint8_t *m_memory = NULL;

float m_fps = 30;
float m_actual_fps = 0;
float m_time = 0.0;

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

int p8_init_file(char *file_name)
{
    p8_init();

#ifdef SDL
    char *lua_script = (char *)malloc(MEMORY_LUA_SIZE);
#else
    char *lua_script = (char *)rh_malloc(MEMORY_LUA_SIZE);
#endif

    memset(lua_script, 0, MEMORY_LUA_SIZE);

    int lua_start, lua_end;

    parse_cart_file(file_name, m_memory, &lua_script, &lua_start, &lua_end);

    lua_load_api();
    lua_init_script(lua_script);

#ifdef SDL
    free(lua_script);
#else
    rh_free(lua_script);
#endif

    clear_screen(0);

    lua_init();

    p8_init_lcd();

    p8_main_loop();

    return 0;
}

int p8_init_ram(uint8_t *buffer, int size)
{
    p8_init();

    /* #ifdef SDL
        char *lua_script = (char *)malloc(MEMORY_LUA_SIZE);
    #else
        char *lua_script = (char *)rh_malloc_psram(MEMORY_LUA_SIZE);
    #endif */

    int lua_start, lua_end;

    parse_cart_ram(buffer, size, m_memory, NULL, &lua_start, &lua_end);

    /* #ifdef SDL
        free(buffer);
    #else
        rh_free(buffer);
    #endif */

    // printf("%s", m_lua_script);

    buffer[lua_end] = '\0';

    // printf("%s\r\n", (char *)(buffer + lua_start));

    lua_load_api();
    lua_init_script((char *)(buffer + lua_start));

    /* #ifdef SDL
        free(lua_script);
    #else
        rh_free(lua_script);
    #endif */

#ifdef SDL
    free(buffer);
#else
    rh_free(buffer);
#endif

    clear_screen(0);

    lua_init();

    p8_init_lcd();

    p8_main_loop();

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
            uint32_t color = m_colors[index & 0xF];

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

                uint8_t index_left = pal[left] & 0xF;
                uint8_t index_right = pal[right] & 0xF;

                uint16_t c_left = m_colors[index_left];
                uint16_t c_right = m_colors[index_right];

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

                index_left = pal[left] & 0xF;
                index_right = pal[right] & 0xF;

                c_left = m_colors[index_left];
                c_right = m_colors[index_right];

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

                index_left = pal[left] & 0xF;
                index_right = pal[right] & 0xF;

                c_left = m_colors[index_left];
                c_right = m_colors[index_right];

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

                index_left = pal[left] & 0xF;
                index_right = pal[right] & 0xF;

                c_left = m_colors[index_left];
                c_right = m_colors[index_right];

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

                uint8_t index_left = pal[left] & 0xF;
                uint8_t index_right = pal[right] & 0xF;

                uint16_t c_left = m_colors[index_left];
                uint16_t c_right = m_colors[index_right];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left] & 0xF;
                index_right = pal[right] & 0xF;

                c_left = m_colors[index_left];
                c_right = m_colors[index_right];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left] & 0xF;
                index_right = pal[right] & 0xF;

                c_left = m_colors[index_left];
                c_right = m_colors[index_right];

                *top++ = c_left;
                *top++ = c_left;
                *top++ = c_right;
                *top++ = c_right;

                screen_mem++;

                left = (*screen_mem) & 0xF;
                right = (*screen_mem) >> 4;

                index_left = pal[left] & 0xF;
                index_right = pal[right] & 0xF;

                c_left = m_colors[index_left];
                c_right = m_colors[index_right];

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
#ifdef OS_FREERTOS
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

void p8_main_loop()
{
    long start_time, end_time;
    float elapsed_time;
#ifdef SDL
    SDL_Event event;
#endif
    bool done = 0;

#ifdef OS_FREERTOS
    start_time = xTaskGetTickCount();
#else
    start_time = clock();
#endif

    while (!done)
    {
#ifdef SDL
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
#endif
        p8_update_input();

        lua_update();
        lua_draw();

        p8_render();

#ifdef OS_FREERTOS
        end_time = xTaskGetTickCount();
        elapsed_time = (float)(end_time - start_time) * portTICK_PERIOD_MS;
        m_time += elapsed_time;

        const float target_frame_time = 1000.0f / m_fps;
        float sleep_time = target_frame_time - elapsed_time;
        m_actual_fps = 1000.0f / (elapsed_time + sleep_time);

        if (sleep_time > 0)
        {
            vTaskDelay(pdMS_TO_TICKS(sleep_time));
        }

        start_time = xTaskGetTickCount();
#else
        end_time = clock();
        elapsed_time = (float)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0f;
        m_time += elapsed_time;

        const float target_frame_time = 1000.0f / m_fps;
        float sleep_time = target_frame_time - elapsed_time;
        m_actual_fps = 1000.0f / (elapsed_time + sleep_time);

        if (sleep_time > 0)
        {
            usleep(sleep_time * 1000);
        }

        start_time = clock();
#endif
    }
}
