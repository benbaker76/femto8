/*
 * main.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <stdio.h>
#include "p8_browse.h"
#include "p8_parser.h"
#include "p8_emu.h"

int main(int argc, char *argv[])
{
    const char *file_name;
    if (argc >= 2)
        file_name = argv[1];
    else
        file_name = browse_for_cart();
    if (file_name != NULL)
        p8_init_file(file_name);
    p8_shutdown();

    return 0;
}
