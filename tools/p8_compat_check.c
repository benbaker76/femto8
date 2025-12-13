/**
 * Copyright (C) 2025 Chris January
 *
 * PICO-8 cart compatibility checker
 * Tests PICO-8 cart files for femto8 compatibility
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "p8_compat.h"
#include "p8_emu.h"
#include "p8_parser.h"

unsigned char *m_cart_memory = NULL;

#define MAX_FILES 10000

static int is_cart_file(const char *filename)
{
    size_t len = strlen(filename);
    if (len >= 3) {
        const char *ext = filename + len - 3;
        if (strcmp(ext, ".p8") == 0 || strcmp(ext, ".P8") == 0)
            return 1;
    }
    if (len >= 7) {
        const char *ext = filename + len - 7;
        if (strcmp(ext, ".p8.png") == 0 || strcmp(ext, ".P8.PNG") == 0)
            return 1;
    }
    return 0;
}

static int add_files_from_directory(const char *dir_path, char **file_list, int file_count)
{
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "%s: %s\n", dir_path, strerror(errno));
        return file_count;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (entry->d_type == DT_REG && is_cart_file(entry->d_name)) {
            size_t path_len = strlen(dir_path) + strlen(entry->d_name) + 2;
            char *full_path = malloc(path_len);
            if (!full_path) {
                write(STDERR_FILENO, "Out of memory\n", 14);
                closedir(dir);
                return file_count;
            }
            snprintf(full_path, path_len, "%s/%s", dir_path, entry->d_name);
            file_list[file_count++] = full_path;
        }
    }

    closedir(dir);
    return file_count;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cart.p8|directory> [cart2.p8|directory2] ...\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Tests PICO-8 cart files for femto8 compatibility.\n");
        fprintf(stderr, "Directories are searched non-recursively for .p8 files.\n");
        return 1;
    }

    char **file_list = malloc(MAX_FILES * sizeof(char *));
    if (!file_list) {
        write(STDERR_FILENO, "Out of memory\n", 14);
        return 1;
    }

    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) != 0) {
            fprintf(stderr, "%s: %s\n", argv[i], strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            file_count = add_files_from_directory(argv[i], file_list, file_count);
        } else if (S_ISREG(st.st_mode)) {
            if (is_cart_file(argv[i])) {
                if (file_count < MAX_FILES) {
                    file_list[file_count++] = strdup(argv[i]);
                }
            } else {
                fprintf(stderr, "%s: not a .p8 or .p8.png file\n", argv[i]);
            }
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "Error: no cart files found\n");
        free(file_list);
        return 1;
    }

    m_cart_memory = malloc(CART_MEMORY_SIZE);
    if (!m_cart_memory) {
        write(STDERR_FILENO, "Out of memory\n", 14);
        for (int i = 0; i < file_count; i++) {
            free(file_list[i]);
        }
        free(file_list);
        return 1;
    }

    int total = 0;
    int compatible = 0;

    for (int i = 0; i < file_count; i++) {
        const char *lua_script = NULL;
        uint8_t *file_buffer = NULL;

        parse_cart_file(file_list[i], m_cart_memory, &lua_script, &file_buffer);
        if (lua_script) {
            total++;
            int result = check_compatibility(file_list[i], lua_script);
            if (result == COMPAT_OK) {
                compatible++;
            }
        }

        if (file_buffer) {
            free(file_buffer);
        }
        free(file_list[i]);
    }

    free(file_list);
    free(m_cart_memory);

    if (total > 0) {
        int percentage = (compatible * 100) / total;
        printf("%d/%d compatible (%d%%)\n", compatible, total, percentage);
    } else {
        fprintf(stderr, "No valid cart files processed\n");
        return 1;
    }

    return 0;
}
