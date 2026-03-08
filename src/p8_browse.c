/**
 * Copyright (C) 2025 Chris January
 */

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#if defined(__APPLE__)
  #include <stdlib.h>
  #include <malloc/malloc.h>
#elif defined(__OpenBSD__)
  #include <stdlib.h>
#else
  #include <malloc.h>
#endif
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "p8_browse.h"
#include "p8_dialog.h"
#include "p8_emu.h"
#include "p8_lua_helper.h"
#include "p8_options.h"
#include "p8_overlay_helper.h"
#include "p8_pause_menu.h"

#define FALLBACK_CARTS_PATH "."

struct dir_entry {
    const char *file_name;
    bool is_dir;
};

static const char *pwd = NULL;
static struct dir_entry *dir_contents = NULL;
static int nitems = 0;
static int capacity = 0;
static int current_item = 0;
static void clear_dir_contents(void) {
    for (int i=0;i<nitems;++i)
        free((char *)dir_contents[i].file_name);
    nitems = 0;
    current_item = 0;
}
static void append_dir_entry(const char *file_name, bool is_dir)
{
    if (nitems == capacity) {
        int new_capacity;
        if (capacity == 0)
            new_capacity = 10;
        else
            new_capacity = capacity * 2;
        struct dir_entry *new_contents = realloc(dir_contents, sizeof(dir_contents[0]) * new_capacity);
        if (!new_contents) {
            fputs("Out of memory\n", stderr);
            return;
        }
        dir_contents = new_contents;
        capacity = new_capacity;
    }
    struct dir_entry *dir_entry = &dir_contents[nitems];
    dir_entry->file_name = strdup(file_name);
    if (!dir_entry->file_name) {
        fputs("Out of memory\n", stderr);
        return;
    }
    dir_entry->is_dir = is_dir;
    nitems++;
}
static const char *make_full_path(const char *dir_path, const char *file_name)
{
    if (!dir_path || !file_name)
        return NULL;

    size_t dir_len = strlen(dir_path);
    size_t file_len = strlen(file_name);
    if (dir_len > 256 || file_len > 256)
        return NULL;

    char *ret = malloc(dir_len + 1 + file_len + 1);
    if (!ret)
        return NULL;

    if (strcmp(file_name, "..") == 0) {
        strcpy(ret, dir_path);
        char *slash = strrchr(ret, '/');
        if (!slash)
            slash = strrchr(ret, '\\');
        if (slash) {
            if (slash[1] == '\0' && ret[1] == ':' && slash==ret+2) {
                // If going up a directory from the root of a drive,
                // go up to the list of drives rather than the current
                // directory of the drive.
                // i.e. "C:\" -> "" rather than "C:\" -> "C:"
                ret[0] = '\0';
            } else if (ret[1] == ':' && slash==ret+2) {
                // If going up to the root of the drive don't erase
                // the trailing slash.
                slash[1] = '\0';
            } else {
                slash[0] = '\0';
            }
        }
    } else {
        strcpy(ret, dir_path);
        if (dir_len > 0 &&
            ret[dir_len - 1] != '/' &&
            ret[dir_len - 1] != '\\')
            strcat(ret, "/");
        strcat(ret, file_name);
    }
    return ret;
}
static int compare_dir_entry(const void *p1, const void *p2)
{
    struct dir_entry *dir_entry1 = (struct dir_entry *)p1;
    struct dir_entry *dir_entry2 = (struct dir_entry *)p2;
    if (dir_entry1->is_dir != dir_entry2->is_dir)
        return (dir_entry1->is_dir ? 0 : 1) - (dir_entry2->is_dir ? 0 : 1);
    else
        return strcmp(dir_entry1->file_name, dir_entry2->file_name);
}
static void list_dir(const char* path) {
#ifdef NEXTP8
    if (path[0] == '\0') {
        free((char *)pwd);
        pwd = strdup(path);
        clear_dir_contents();
        // Show drives
        static const char *volume_names[2] = {"0:/", "1:/"};
        for (int i=0;i<2;++i)
            append_dir_entry(volume_names[i], true);
        return;
    }
#endif
    p8_show_io_icon(true);
    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
    } else {
        free((char *)pwd);
        pwd = strdup(path);
        clear_dir_contents();
        for (;;) {
            struct dirent *dirent;
            errno = 0;
            dirent = readdir(dir);
            if (dirent == NULL) {
                if (errno != 0)
                    fprintf(stderr, "%s: %s\n", path, strerror(errno));
                break;
            }
            const char *full_path = make_full_path(path, dirent->d_name);
            if (!full_path) {
                fputs("Out of memory\n", stderr);
                continue;
            }
            bool is_dir = false;
            if (full_path[0] == '\0' ||
                (full_path[1] == ':' &&
                 (full_path[2] == '/' || full_path[2] == '\\') &&
                 full_path[3] == '\0')) {
                is_dir = true;
            } else {
                struct stat statbuf;
                int res = stat(full_path, &statbuf);
                if (res == 0)
                    is_dir = S_ISDIR(statbuf.st_mode);
                else
                    fprintf(stderr, "%s: %s\n", full_path, strerror(errno));
            }
            free((char *)full_path);
            append_dir_entry(dirent->d_name, is_dir);
        }
        closedir(dir);
    }
    qsort(dir_contents, nitems, sizeof(dir_contents[0]), compare_dir_entry);
    p8_show_io_icon(false);
}
static void draw_file_name(const char *str, int x, int y, int col)
{
    int cursor_x = x;
    for (const char *c = str; *c != '\0'; c++) {
        if (*c >= 0x20 && *c < 0x7F) {
            if (*c >= 'a' && *c <= 'z') {
                overlay_draw_char(*c - ('a' - 'A'), cursor_x, y, col);
            } else if (*c >= 'A' && *c <= 'Z') {
                overlay_draw_char(*c + ('a' - 'A'), cursor_x, y, col);
            } else {
                overlay_draw_char(*c, cursor_x, y, col);
            }
            cursor_x += GLYPH_WIDTH;
        }
    }
}

static void render_file_item(void *user_data, int index, bool selected, int x, int y, int width, int height, int fg_color, int bg_color)
{
    (void)user_data;
    struct dir_entry *dir_entry = &dir_contents[index];

    if (selected)
        overlay_draw_rectfill(x, y - 1, x + width - 1, y + height - 1, bg_color);

    // // Clip to avoid drawing filename over " <dir>" suffix
    int clip_x, clip_y, clip_w, clip_h;
    overlay_clip_get(&clip_x, &clip_y, &clip_w, &clip_h);
    if (dir_entry->is_dir)
        overlay_clip_set(x, y, width - GLYPH_WIDTH * 6, height);
    else
        overlay_clip_set(x, y, width, height);

    // Draw filename with case conversion
    draw_file_name(dir_entry->file_name, x, y, fg_color);

    overlay_clip_set(clip_x, clip_y, clip_w, clip_h);

    // Draw " <dir>" suffix for directories
    if (dir_entry->is_dir)
        overlay_draw_simple_text(" <dir>", x + width - GLYPH_WIDTH * 6, y, fg_color);
}

static int show_menu(void)
{
    p8_dialog_control_t controls[] = {
        DIALOG_MENUITEM("show version", 1),
        DIALOG_MENUITEM("controls", 2),
        DIALOG_MENUITEM("back", 3)
    };

    p8_dialog_t dialog;
    p8_dialog_init(&dialog, NULL, controls, sizeof(controls) / sizeof(controls[0]), 0);
    p8_dialog_action_t result = p8_dialog_run(&dialog);
    p8_dialog_cleanup(&dialog);

    if (result.type == DIALOG_RESULT_BUTTON)
        return result.action_id;
    else
        return 0;
}

const char *browse_for_cart(void)
{
    p8_init();
    p8_reset();
    if (setjmp(jmpbuf_restart)) {
        return NULL;
    }

    if (access(DEFAULT_CARTS_PATH, F_OK) == 0)
        list_dir(DEFAULT_CARTS_PATH);
    else
        list_dir(FALLBACK_CARTS_PATH);

    const char *cart_path = NULL;
    int selected_index = 0;

    // Create dialog with custom listbox renderer
    p8_dialog_control_t controls[] = {
        DIALOG_LABEL_INVERTED(""),
        DIALOG_LISTBOX_CUSTOM_FULLSCREEN(NULL, NULL, nitems, &selected_index, render_file_item),
        DIALOG_LABEL_INVERTED("\216: select file  \227: menu"),
    };

    p8_dialog_t dialog;
    p8_dialog_init(&dialog, NULL, controls, sizeof(controls) / sizeof(controls[0]), P8_WIDTH);
    dialog.draw_border = false;
    dialog.padding = 0;

    p8_dialog_action_t result = { DIALOG_RESULT_NONE, 0 };
    p8_dialog_set_showing(&dialog, true);

    for (;;) {
        selected_index = 0;
        controls[0].label = pwd;
        controls[1].data.listbox.item_count = nitems;

        // Main dialog loop
        do {
            p8_dialog_draw(&dialog);
            p8_flip();

            result = p8_dialog_update(&dialog);

            if (result.type == DIALOG_RESULT_NONE && ((m_buttonsp[0] & BUTTON_MASK_ACTION2) != 0)) {
                int action_id = show_menu();
                switch (action_id) {
                    case 1:
                        p8_show_version_dialog();
                        break;
                    case 2:
                        p8_show_controls_dialog();
                        break;
                    default:
                        break;
                }
                if (action_id == 2 && cart_path)
                    break;
            }
        } while (result.type == DIALOG_RESULT_NONE);

        if (cart_path)
            break;

        if (result.type == DIALOG_RESULT_CANCELLED)
            break;

        if (result.type == DIALOG_RESULT_ACCEPTED && selected_index >= 0 && selected_index < nitems) {
            struct dir_entry *dir_entry = &dir_contents[selected_index];
            const char *full_path = make_full_path(pwd, dir_entry->file_name);
            if (!full_path) {
                fputs("Out of memory\n", stderr);
                continue;
            }
            if (dir_entry->is_dir) {
                list_dir(full_path);
                free((char *)full_path);
                selected_index = 0;
            } else {
                cart_path = full_path;
                break;
            }
        }
    }

    p8_dialog_set_showing(&dialog, false);
    p8_dialog_cleanup(&dialog);

    overlay_clear();
    p8_flip();

    clear_dir_contents();
    free(dir_contents);
    free((char *)pwd);

    return cart_path;
}
