/*
 * main.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <stdio.h>
#include <string.h>
#include "p8_browse.h"
#include "p8_parser.h"
#include "p8_emu.h"

int main(int argc, char *argv[])
{
    const char *file_name = NULL;
    const char *param_string = NULL;
    bool skip_compat = false;
    bool skip_main_loop = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--skip-compat-check") == 0) {
            skip_compat = true;
        } else if (strcmp(argv[i], "-x") == 0) {
            skip_main_loop = true;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            param_string = argv[++i];
        } else if (file_name == NULL) {
            file_name = argv[i];
        }
    }

    if (file_name == NULL)
        file_name = browse_for_cart();

    if (skip_compat)
        p8_set_skip_compat_check(true);
    if (skip_main_loop)
        p8_set_skip_main_loop_if_no_callbacks(true);
    if (file_name != NULL)
        p8_init_file_with_param(file_name, param_string);
    p8_shutdown();

    return 0;
}
