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
#include "p8_emu.h"
#include "p8_lua_helper.h"
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
                draw_char(*c - ('a' - 'A'), cursor_x, y, col);
            } else if (*c >= 'A' && *c <= 'Z') {
                draw_char(*c + ('a' - 'A'), cursor_x, y, col);
            } else {
                draw_char(*c, cursor_x, y, col);
            }
            cursor_x += GLYPH_WIDTH;
        }
    }
}

#define HEADER_HEIGHT GLYPH_HEIGHT
#define FOOTER_HEIGHT GLYPH_HEIGHT
#define LIST_TOP      HEADER_HEIGHT
#define LIST_HEIGHT   (P8_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT)
#define FOOTER_TOP    (P8_HEIGHT - FOOTER_HEIGHT)

static void display_dir_contents()
{
    clear_screen(1);
    draw_rectfill(0, 0, P8_WIDTH, GLYPH_HEIGHT - 1, 7, 0);
    if (pwd != NULL) draw_file_name(pwd, 1, 0, 1);
    draw_rectfill(0, FOOTER_TOP, P8_WIDTH - 1, P8_HEIGHT - 1, 7, 0);
    draw_simple_text("z/fire: select file", 1, FOOTER_TOP, 1);
    clip_set(0, LIST_TOP, P8_WIDTH, LIST_HEIGHT);
    int y = LIST_TOP;
    int scroll = current_item * GLYPH_HEIGHT + (GLYPH_HEIGHT - LIST_HEIGHT) / 2;
    if (scroll > nitems * GLYPH_HEIGHT - LIST_HEIGHT) scroll = nitems * GLYPH_HEIGHT - LIST_HEIGHT;
    if (scroll < 0) scroll = 0;
    for (int i=0;i<nitems;++i) {
        struct dir_entry *dir_entry = &dir_contents[i];
        if (y - scroll > -GLYPH_HEIGHT && y - scroll < P8_HEIGHT) {
            bool highlighted = (current_item == i);
            if (highlighted)
                draw_rectfill(0, y - scroll, P8_WIDTH - 1, y - scroll + GLYPH_HEIGHT - 1, 10, 0);
            if (dir_entry->is_dir)
                clip_set(0, LIST_TOP, P8_WIDTH - GLYPH_WIDTH * 6, LIST_HEIGHT);
            int fg = highlighted ? 1 : 7;
            draw_file_name(dir_entry->file_name, 1, y - scroll, fg);
            if (dir_entry->is_dir)
                clip_set(0, LIST_TOP, P8_WIDTH, LIST_HEIGHT);
            if (dir_entry->is_dir) {
                draw_simple_text(" <dir>", 128 - GLYPH_WIDTH * 6, y - scroll, fg);
            }
        }
        y += GLYPH_HEIGHT;
    }
}

const char *browse_for_cart(void)
{
    p8_init();
    m_pause_menu_showing = true;
    p8_reset();
    if (setjmp(jmpbuf_restart)) {
        m_pause_menu_showing = false;
        return NULL;
    }
    display_dir_contents();
    p8_flip();
    if (access(DEFAULT_CARTS_PATH, F_OK) == 0)
        list_dir(DEFAULT_CARTS_PATH);
    else
        list_dir(FALLBACK_CARTS_PATH);
    uint16_t buttons = 0;
    const char *cart_path = NULL;
    for (;;) {
        buttons = m_buttonsp[0];
        if (buttons & BUTTON_MASK_UP) {
            if (current_item > 0)
                current_item--;
        }
        if (buttons & BUTTON_MASK_DOWN) {
            if (current_item < nitems - 1)
                current_item++;
        }
        if (buttons & (BUTTON_MASK_ACTION1 | BUTTON_MASK_RETURN)) {
            struct dir_entry *dir_entry = &dir_contents[current_item];
            const char *full_path = make_full_path(pwd, dir_entry->file_name);
            if (!full_path) {
                fputs("Out of memory\n", stderr);
                continue;
            }
            if (dir_entry->is_dir) {
                list_dir(full_path);
                free((char *)full_path);
            } else {
                cart_path = full_path;
                break;
            }
        }
        display_dir_contents();
        p8_flip();
    }
    clear_dir_contents();
    free(dir_contents);
    free((char *)pwd);
    m_button_down_time[0][BUTTON_ACTION1] = UINT_MAX;
    m_pause_menu_showing = false;
    return cart_path;
}
