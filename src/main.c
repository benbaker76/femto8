/*
 * main.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p8_browse.h"
#include "p8_parser.h"
#include "p8_emu.h"

#define VERSION "1.0.00"

const char *femto8_version = VERSION;

int main(int argc, char *argv[])
{
    const char *file_name = NULL;
    const char *param_string = NULL;
    bool skip_main_loop = false;
    int exit_code = EXIT_SUCCESS;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("v%s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--skip-compat-check") == 0) {
            // Ignore for compatibiliy
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

    if (skip_main_loop)
        p8_set_skip_main_loop_if_no_callbacks(true);
    if (file_name != NULL) {
        if (p8_init_file_with_param(file_name, param_string) != 0)
            exit_code = EXIT_FAILURE;
    }
    p8_shutdown();

    return exit_code;
}
