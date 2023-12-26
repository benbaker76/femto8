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
