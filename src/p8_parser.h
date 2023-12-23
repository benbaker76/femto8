/*
 * p8_parser.h
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#ifndef P8_PARSER_H
#define P8_PARSER_H

#include <stdint.h>

void parse_cart_ram(uint8_t *buffer, int size, uint8_t *memory, char **lua_script, int *lua_start, int *lua_end);
void parse_cart_file(char *file_name, uint8_t *memory, char **lua_script, int *lua_start, int *lua_end);

#endif
