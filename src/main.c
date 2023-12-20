/*
 * main.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <stdio.h>
#include "p8_parser.h"
#include "p8_emu.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <p8_file>\n", argv[0]);
        return 1;
    }

    char *file_name = argv[1];

    p8_init_file(file_name);
    p8_shutdown();

    return 0;
}
