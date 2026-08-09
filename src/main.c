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
#include "p8_lua.h"
#include "strtcpy.h"

#define VERSION "1.0.00"

const char *femto8_version = VERSION;

int main(int argc, char *argv[])
{
    char file_name[PATH_MAX] = {0};
    const char *param_string = NULL;
    bool skip_main_loop = false;
    int exit_code = EXIT_SUCCESS;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("v%s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--skip-compat-check") == 0) {
            // Flag accepted but currently no compat check logic to skip
        } else if (strcmp(argv[i], "-x") == 0) {
            skip_main_loop = true;
        } else if (strcmp(argv[i], "--no-frame-delay") == 0) {
            m_no_frame_delay = true;
        } else if (strcmp(argv[i], "--no-frame-skip") == 0) {
            m_no_frame_skip = true;
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -p flag requires an argument\n");
                return EXIT_FAILURE;
            }
            param_string = argv[++i];
        } else if (file_name[0] == '\0') {
            if (strtcpy(file_name, argv[i], PATH_MAX) < 0) {
                fputs("Path too long\n", stderr);
                return EXIT_FAILURE;
            }
        }
    }

    if (p8_init() != 0)
        return EXIT_FAILURE;

    if (file_name[0] == '\0') {
        if (browse_for_cart(file_name, sizeof(file_name)) < 0)
            return EXIT_FAILURE;
    }

    if (skip_main_loop)
        p8_set_skip_main_loop_if_no_callbacks(true);
    if (file_name[0] != '\0') {
        if (p8_load(file_name, param_string, NULL, NULL) != 0) {
            exit_code = EXIT_FAILURE;
        } else {
            if (p8_run() != 0) {
                lua_print_error();
                exit_code = EXIT_FAILURE;
            }
        }
    }
    p8_shutdown();

    return exit_code;
}
