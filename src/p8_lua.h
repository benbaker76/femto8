/*
 * p8_lua.h
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#ifndef P8_LUA_H
#define P8_LUA_H

#include <stdint.h>
#include <stdbool.h>

void clear_screen();
void draw_circ(int x, int y, int r, int col);
void draw_circfill(int x, int y, int r, int col);
void draw_sprite(int n, int left, int top, bool flip_x, bool flip_y);
void draw_scaled_sprite(int sx, int sy, int sw, int sh, int dx, int dy, float scale_x, float scale_y, bool flip_x, bool flip_y);
void draw_sprites(int n, int x, int y, float w, float h, bool flip_x, bool flip_y);
void draw_line(int x0, int y0, int x1, int y1, int col);
void print_char(int n, int left, int top, int col);
uint8_t gfx_get(int x, int y, int location, int size);
void gfx_set(int x, int y, int location, int size, int col);
void camera_get(int *x, int *y);
void camera_set(int x, int y);
uint8_t pencolor_get();
void pencolor_set(uint8_t col);
uint8_t color_get(int type, int index);
void color_set(int type, int index, int col);
void clip_set(int x, int y, int w, int h);
void cursor_get(int *x, int *y);
void cursor_set(int x, int y, int col);
void pixel_set(int x, int y, int c);
int draw_text(const char *str, int x, int y, int col);
bool is_button_set(int index, int button, bool prev_buttons);
void update_buttons(int index, int button, bool state);
void map_set(int celx, int cely, int snum);
uint8_t map_get(int celx, int cely);
void reset_color();

void lua_load_api();
void lua_shutdown_api();
void lua_print_error(const char *where);
void lua_init_script(const char *script);
void lua_call_function(const char *name, int ret);
void lua_update();
void lua_draw();
void lua_init();

extern char m_str_buffer[256];

#endif
