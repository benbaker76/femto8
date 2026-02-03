/**
 * Copyright (C) 2025 Chris January
 */

#include <stdbool.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "p8_pause_menu.h"
#include "p8_emu.h"
#include "p8_dialog.h"

void p8_show_pause_menu(void)
{
    if (m_dialog_showing)
        return;
    m_dialog_showing = true;

    p8_dialog_control_t pause_controls[] = {
        DIALOG_BUTTON("continue", 0),
        DIALOG_BUTTON("restart", 1),
        DIALOG_BUTTON("quit", 2),
    };

    p8_dialog_t pause_dialog;
    p8_dialog_init(&pause_dialog, NULL, pause_controls, 3, P8_WIDTH / 2);

    p8_dialog_action_t result = p8_dialog_run(&pause_dialog);
    p8_dialog_cleanup(&pause_dialog);

    m_dialog_showing = false;

    switch (result.action_id) {
        case 0: // Continue
            break;
        case 1: // Restart
            p8_restart();
            break;
        case 2: // Quit
            p8_abort();
            break;
    }

    return;
}
