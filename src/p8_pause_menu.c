/**
 * Copyright (C) 2025 Chris January
 */

#include <stdbool.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "p8_pause_menu.h"
#include "p8_emu.h"
#include "p8_lua_helper.h"

#define MENU_ITEMS 3
static const char *menu_items[MENU_ITEMS] = {
    "continue",
    "restart",
    "quit"
};

#define MENU_WIDTH (P8_WIDTH / 2)
#define MENU_ITEM_HEIGHT (GLYPH_HEIGHT + 2)
#define MENU_HEIGHT (MENU_ITEMS * MENU_ITEM_HEIGHT + 4)
#define MENU_X ((P8_WIDTH - MENU_WIDTH) / 2)
#define MENU_Y ((P8_HEIGHT - MENU_HEIGHT) / 2)

bool m_pause_menu_showing;

static void draw_pause_menu(int current_item)
{
    draw_rect(MENU_X - 2, MENU_Y - 2, MENU_X + MENU_WIDTH + 1, MENU_Y + MENU_HEIGHT + 1, 1, 0);
    draw_rect(MENU_X - 1, MENU_Y - 1, MENU_X + MENU_WIDTH, MENU_Y + MENU_HEIGHT, 7, 0);
    draw_rectfill(MENU_X, MENU_Y, MENU_X + MENU_WIDTH - 1, MENU_Y + MENU_HEIGHT - 1, 1, 0);
    
    for (int i = 0; i < MENU_ITEMS; i++) {
        int item_y = MENU_Y + 2 + i * MENU_ITEM_HEIGHT;
        bool highlighted = (current_item == i);
        
        if (highlighted) {
            draw_rectfill(MENU_X + 1, item_y, MENU_X + MENU_WIDTH - 2, 
                         item_y + MENU_ITEM_HEIGHT - 1, 10, 0);
        }
        
        int fg = highlighted ? 1 : 7;
        draw_simple_text(menu_items[i], MENU_X + 3, item_y + 1, fg);
    }
}

void p8_show_pause_menu(void)
{
    if (m_pause_menu_showing)
        return;
    m_pause_menu_showing = true;

    m_overlay_enabled = true;
    m_overlay_transparent_color = 0;

    memset(m_overlay_memory, 0, MEMORY_SCREEN_SIZE);

    int current_item = 0;
    draw_pause_menu(current_item);
    p8_flip();
    for (;;) {
        uint16_t buttons = m_buttonsp[0];
        if (buttons & BUTTON_MASK_UP) {
            if (current_item > 0)
                current_item--;
        }
        if (buttons & BUTTON_MASK_DOWN) {
            if (current_item < MENU_ITEMS - 1)
                current_item++;
        }
        if (buttons & BUTTON_MASK_RETURN)
            break;

        draw_pause_menu(current_item);
        p8_flip();
    }

    m_overlay_enabled = false;
    m_pause_menu_showing = false;
    p8_flip();

    switch (current_item) {
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
