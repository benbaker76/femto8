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
#include "p8_options.h"
#include "p8_audio.h"

void p8_show_pause_menu(void)
{
    if (m_dialog_showing)
        return;

    // 0x5f30: suppress next pause menu activation
    if (m_memory[MEMORY_SUPPRESS_PAUSE] == 1) {
        m_memory[MEMORY_SUPPRESS_PAUSE] = 0;
        return;
    }

    m_dialog_showing = true;

    // Pause audio during pause menu unless 0x5f2f == 2
    bool should_pause_audio = m_memory[MEMORY_AUDIO_PAUSE] != 2;
    if (should_pause_audio)
        audio_pause();

    p8_dialog_control_t pause_controls[] = {
        DIALOG_BUTTON("continue", 0),
        DIALOG_BUTTON("restart", 1),
        DIALOG_BUTTON("show version", 2),
        DIALOG_BUTTON("controls", 3),
        DIALOG_BUTTON("quit", 4),
    };

    p8_dialog_t pause_dialog;
    p8_dialog_init(&pause_dialog, NULL, pause_controls, 5, P8_WIDTH / 2);

    p8_dialog_action_t result = p8_dialog_run(&pause_dialog);
    p8_dialog_cleanup(&pause_dialog);

    m_dialog_showing = false;

    if (should_pause_audio)
        audio_resume();

    switch (result.action_id) {
        case 0: // Continue
            break;
        case 1: // Restart
            p8_restart();
            break;
        case 2: // Show version
            p8_show_version_dialog();
            break;
        case 3: // Controls
            p8_show_controls_dialog();
            break;
        case 4: // Quit
            p8_abort();
            break;
    }

    return;
}
