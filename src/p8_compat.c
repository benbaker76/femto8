/**
 * Copyright (C) 2025 Chris January
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p8_compat.h"
#include "p8_emu.h"
#include "p8_parser.h"

const char *p8_functions[] = {
    "abs",
    "add",
    "all",
    "assert",
    "atan",
    "band",
    "bnot",
    "bor",
    "btn",
    "btnp",
    "bxor",
    "camera",
    "cartdata",
    "ceil",
    "chr",
    "circ",
    "circfill",
    "clip",
    "cls",
    "cocreate",
    "color",
    "coresume",
    "cos",
    "costatus",
    "count",
    "cstore",
    "cursor",
    "del",
    "deli",
    "dget",
    "dset",
    "extcmd",
    "fget",
    "fillp",
    "flip",
    "flr",
    "foreach",
    "fset",
    "function",
    "getmetatable",
    "info",
    "line",
    "load",
    "ls",
    "lshr",
    "map",
    "max",
    "memcpy",
    "memset",
    "menuitem",
    "mget",
    "mid",
    "min",
    "mset",
    "music",
    "ord",
    "oval",
    "ovalfill",
    "pairs",
    "pal",
    "palt",
    "peek",
    "pget",
    "poke",
    "print",
    "printh",
    "pset",
    "rawequal",
    "rawget",
    "rawlen",
    "rawset",
    "rect",
    "rectfill",
    "reload",
    "reset",
    "rnd",
    "rotl",
    "rotr",
    "rrect",
    "rrectfill",
    "run",
    "save",
    "select",
    "serial",
    "setmetatable",
    "sfx",
    "sget",
    "sgn",
    "shl",
    "shr",
    "sin",
    "split",
    "spr",
    "sqrt",
    "srand",
    "sset",
    "sspr",
    "stat",
    "stop",
    "sub",
    "time",
    "tline",
    "tonum",
    "tostr",
    "type",
    "yield",
    NULL
};

const char *p8_lua_functions[] = {
    "btn",
    "btnp",
    "camera",
    "cartdata",
    "circ",
    "circfill",
    "clip",
    "cls",
    "color",
    "cursor",
    "dget",
    "dset",
    "fget",
    "fillp",
    "flip",
    "fset",
    "line",
    "map",
    "memcpy",
    "memset",
    "menuitem",
    "mget",
    "mset",
    "music",
    "oval",
    "ovalfill",
    "pal",
    "palt",
    "peek",
    "pget",
    "poke",
    "print",
    "printh",
    "pset",
    "rect",
    "rectfill",
    "rrect",
    "rrectfill",
    "reload",
    "reset",
    "rnd",
    "run",
    "sfx",
    "sget",
    "spr",
    "srand",
    "sset",
    "sspr",
    "stat",
    "sub",
    "t",
    "time",
    "tline",
    NULL
};

const char *lua_lib_functions[] = {
    "abs",
    "assert",
    "band",
    "bnot",
    "bor",
    "bxor",
    "call",
    "ceil",
    "chr",
    "collect",
    "collectgarbage",
    "concat",
    "cos",
    "count",
    "create",
    "debug",
    "dofile",
    "error",
    "flr",
    "gethook",
    "getinfo",
    "getlocal",
    "getmetatable",
    "getregistry",
    "getupvalue",
    "getuservalue",
    "insert",
    "ipairs",
    "line",
    "load",
    "loadfile",
    "loadstring",
    "lshr",
    "max",
    "maxn",
    "mid",
    "min",
    "next",
    "ord",
    "pack",
    "pairs",
    "pcall",
    "print",
    "rawequal",
    "rawget",
    "rawlen",
    "rawset",
    "remove",
    "restart",
    "resume",
    "return",
    "rotl",
    "rotr",
    "running",
    "select",
    "sethook",
    "setlocal",
    "setmetatable",
    "setupvalue",
    "setuservalue",
    "sgn",
    "shl",
    "shr",
    "sin",
    "sort",
    "split",
    "sqrt",
    "status",
    "stop",
    "tonum",
    "tonumber",
    "tostr",
    "tostring",
    "traceback",
    "type",
    "unpack",
    "upvalueid",
    "upvaluejoin",
    "wrap",
    "xpcall",
    "yield",
    NULL,
};

const char *lua_api_functions[] = {
    "all",
    "function",
    "function",
    "add",
    "foreach",
    "mapdraw",
    "count",
    "del",
    "deli",
    "cocreate",
    "yield",
    "coresume",
    "costatus",
    NULL,
};

#define HASH_SET_CAPACITY 37

struct hash_set_element {
    struct hash_set_element *next;
    char *value;
};

typedef struct hash_set_element hash_set[HASH_SET_CAPACITY];

static bool hash_sets_initialized;
static hash_set p8_functions_hash_set;
static hash_set supported_functions;

static unsigned hash(const char *value)
{
    unsigned ret = 0;
    for (;*value;value++) {
        ret = (ret << 2) | (ret >> (sizeof(unsigned)*__CHAR_BIT__-2));
        ret ^= *value;
    }
    return ret % HASH_SET_CAPACITY;
}

#if 0
static void hash_set_dump(hash_set *set)
{
    for (int i=0;i<HASH_SET_CAPACITY;++i) {
        struct hash_set_element *el = &(*set)[i];
        printf("[%d] ", i);
        while (el) {
            printf(" -> \"%s\"", el->value ? el->value : "NULL");
            el = el->next;
        }
        printf("\n");
    }
    fflush(stdout);
}
#endif

static void hash_set_init(hash_set *set)
{
    memset(set, 0, sizeof(*set));
}

static void hash_set_add(hash_set *set, const char *value)
{
    unsigned h = hash(value);
    struct hash_set_element *prev = NULL, *el;
    for (el = &(*set)[h]; el && el->value; el=el->next) {
        prev = el;
        if (el->value && strcmp(el->value, value) == 0)
            return;
    }
    if (!el)
        el = calloc(1, sizeof(struct hash_set_element));
    assert(el->value == NULL);
    el->value = strdup(value);
    if (prev)
        prev->next = el;
}

static void hash_set_add_list(hash_set *set, const char **list)
{
    for (const char **p = list; *p; p++)
        hash_set_add(set, *p);
}

static bool hash_set_contains(hash_set *set, const char *value)
{
    unsigned h = hash(value);
    for (struct hash_set_element *el = &(*set)[h]; el; el=el->next) {
        if (el->value && strcmp(el->value, value) == 0)
            return true;
    }
    return false;
}

static void hash_set_free(hash_set *set)
{
    for (unsigned i=0;i<sizeof(*set) / sizeof((*set)[0]);++i) {
        struct hash_set_element *el = &(*set)[i];
        while (el) {
            struct hash_set_element *next = el->next;
            free(el->value);
            if (el != &(*set)[i])
                free(el);
            el = next;
        }
    }
}

static void init_hash_sets()
{
    if (hash_sets_initialized)
        return;
    hash_sets_initialized = true;
    hash_set_init(&p8_functions_hash_set);
    hash_set_init(&supported_functions);
    hash_set_add_list(&p8_functions_hash_set, p8_functions);
    hash_set_add_list(&supported_functions, p8_lua_functions);
    hash_set_add_list(&supported_functions, lua_lib_functions);
    hash_set_add_list(&supported_functions, lua_api_functions);
}

bool is_unsupported_function(const char *function, hash_set *user_defined_functions) {
    if (hash_set_contains(user_defined_functions, function))
        return false;
    if (!hash_set_contains(&p8_functions_hash_set, function))
        return false;
    if (hash_set_contains(&supported_functions, function))
        return false;
    return true;
}

bool string_to_address(const char *address_str, unsigned *ret)
{
    char *endptr = NULL;
    long address = strtol(address_str, &endptr, 0);
    if (endptr == address_str)
        return false;
    *ret = (unsigned)address;
    return true;
}

bool is_unsupported_address(unsigned address) {
    if (address < 0x5f00 || address >= 0x6000)
        return false;
    if ((address >= MEMORY_CLIPRECT && address < MEMORY_CLIPRECT + 4) ||
        address == MEMORY_LEFT_MARGIN ||
        address == MEMORY_PENCOLOR ||
        (address >= MEMORY_CURSOR && address < MEMORY_CURSOR + 2) ||
        (address >= MEMORY_CAMERA && address < MEMORY_CAMERA + 4) ||
        address == MEMORY_LINE_VALID ||
        (address >= MEMORY_LINE_X && address < MEMORY_LINE_X + 2) ||
        (address >= MEMORY_LINE_Y && address < MEMORY_LINE_Y + 2) ||
        address == MEMORY_DEVKIT_MODE ||
        address == 0x5f2e /* unimplemented, but causes no compatibility issues */ ||
        address == MEMORY_AUTO_REPEAT_DELAY ||
        address == MEMORY_AUTO_REPEAT_INTERVAL ||
        (address >= MEMORY_FILLP && address < MEMORY_FILLP + 2) ||
        address == MEMORY_FILLP_ATTR ||
        address == MEMORY_COLOR_FILLP ||
        address == MEMORY_TLINE_MASK_X ||
        address == MEMORY_TLINE_MASK_Y ||
        address == MEMORY_TLINE_OFFSET_X ||
        address == MEMORY_TLINE_OFFSET_Y ||
        address == MEMORY_SCREEN_PHYS ||
        address == MEMORY_SPRITE_PHYS ||
        address == MEMORY_MAP_START ||
        address == MEMORY_MAP_WIDTH ||
        address == 0x5f30 /* unimplemented, but causes no compatibility issues */ ||
        (address >= MEMORY_PALETTES && address < MEMORY_PALETTES + 32) ||
        (address >= MEMORY_RNG_STATE && address < MEMORY_RNG_STATE + 8) ||
        address == MEMORY_TEXT_ATTRS ||
        address == MEMORY_TEXT_CHAR_SIZE ||
        address == MEMORY_TEXT_CHAR_SIZE2 ||
        address == MEMORY_TEXT_OFFSET ||
        address == MEMORY_MISCFLAGS)
        return false;
    return true;
}

bool is_unsupported_stat(unsigned address) {
    if (address == STAT_MEM_USAGE ||
        address == STAT_CPU_USAGE ||
        address == STAT_SYSTEM_CPU_USAGE ||
        address == STAT_FRAMERATE ||
        address == STAT_KEY_PRESSED ||
        address == STAT_KEY_NAME ||
        address == STAT_MOUSE_X ||
        address == STAT_MOUSE_Y  ||
        address == STAT_MOUSE_BUTTONS ||
        address == STAT_MOUSE_XREL ||
        address == STAT_MOUSE_YREL ||
        (address >= 16 && address <= 26) ||
        (address >= 46 && address <= 56) ||
        address == STAT_PARAM ||
        (address >= STAT_YEAR && address <= STAT_SECOND))
        return false;

    return true;
}

int check_compatibility(const char *filename, const char *lua_script)
{
    int ret = 0;
    enum { DEFAULT, DEFAULT_NO_PEEK_POKE_UNOP, NAME, FUNCTION, LBRACKET, PEEK_POKE, STAT, COMMENT1, COMMENT2, STRING, ESCAPE} state = DEFAULT;
    char token[256] = {0};
    char *token_p = token;
    char *token_limit = token + sizeof(token);
    char reported_addresses[0x6000-0x5f00];
    char reported_stats[256];
    init_hash_sets();
    hash_set user_defined_functions, reported_functions;
    hash_set_init(&user_defined_functions);
    hash_set_init(&reported_functions);
    memset(&reported_addresses, 0, sizeof(reported_addresses));
    memset(&reported_stats, 0, sizeof(reported_stats));
    for (;;) {
        char c = *lua_script++;
        if (c == '\0')
            break;
        switch (state) {
deflt:
        case DEFAULT:
            if (c == '@' || c == '%' || c == '$') {
                //printf(" PEEK_OP:%c ", c);
                state = PEEK_POKE;
            } else if (c == ')') {
                state = DEFAULT_NO_PEEK_POKE_UNOP;
            } else {
                goto default_no_peek_poke_unop;
            }
            break;
        case DEFAULT_NO_PEEK_POKE_UNOP:
            state = DEFAULT;
default_no_peek_poke_unop:
            if ((c >= 'a' && c <= 'z') || c == '_') {
                state = NAME;
                goto name;
            } else if (c == '-') {
                state = COMMENT1;
            } else if (c == '"') {
                state = STRING;
            }
            //if (c != ' ' && c != '\t' && c != '\n')
            //    printf(" %c ", c);
            break;
        case NAME:
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
                *token_p = '\0';
                //printf(" NAME:%s", token);
                token_p = token;
                if (strcmp(token, "function") == 0) {
                    state = FUNCTION;
                    break;
                } else {
                    state = LBRACKET;
                    goto lbracket;
                }
            }
name:
            *token_p++ = c;
            if (token_p == token_limit) {
                token_p = token;
                state = DEFAULT;
            }
            break;
        case LBRACKET:
lbracket:
            if (c == ' ' || c == '\t' || c == '\n') {
                // ignore
                break;
            } else if (c == '(') {
                // function call
                //printf(" FUNCTION:%s", token);
                if (is_unsupported_function(token, &user_defined_functions)) {
                    if (!hash_set_contains(&reported_functions, token)) {
                        if (filename)
                            fprintf(stderr, "%s: ", filename);
                        fprintf(stderr, "%s: function is not supported\n", token);
                        hash_set_add(&reported_functions, token);
                        if (ret < COMPAT_NONE) ret = COMPAT_NONE;
                    }
                }
                if (strncmp(token, "peek", 4) == 0 || strncmp(token, "poke", 4) == 0)
                    state = PEEK_POKE;
                else if (strcmp(token, "stat") == 0)
                    state = STAT;
                else
                    state = DEFAULT;
            } else {
                state = DEFAULT_NO_PEEK_POKE_UNOP;
                goto default_no_peek_poke_unop;
            }
            break;
        case PEEK_POKE:
        case STAT:
            if (!((c >= '0' && c <= '9') || (token_p != token && ((c >= 'a' && c <= 'f') || c == 'x')))) {
                if (token_p != token) {
                    *token_p = '\0';
                    //printf(" PEEK_POKE:%s", token);
                    unsigned address = 0;
                    if (string_to_address(token, &address)) {
                        if (state == PEEK_POKE) {
                            if (address >= 0 && address < 0x10000 && is_unsupported_address(address)) {
                                if (!reported_addresses[address - 0x5f00]) {
                                    if (filename)
                                        fprintf(stderr, "%s: ", filename);
                                    fprintf(stderr, "0x%x: peek/poke address is not supported\n", address);
                                    reported_addresses[address - 0x5f00] = 1;
                                    if (ret < COMPAT_SOME) ret = COMPAT_SOME;
                                }
                            }
                        } else if (state == STAT) {
                            if (address >=0 && address < 256 && is_unsupported_stat(address)) {
                                if (!reported_stats[address]) {
                                    if (filename)
                                        fprintf(stderr, "%s: ", filename);
                                    fprintf(stderr, "%d: stat value is not supported\n", address);
                                    reported_stats[address] = 1;
                                    if (ret < COMPAT_SOME) ret = COMPAT_SOME;
                                }
                            }
                        }
                    }
                    token_p = token;
                }
                state = DEFAULT;
                goto deflt;
            }
            *token_p++ = c;
            if (token_p == token_limit) {
                token_p = token;
                state = DEFAULT;
            }
            break;
        case COMMENT1:
            if (c == '-') {
                state = COMMENT2;
                break;
            }
            state = DEFAULT;
            goto deflt;
        case COMMENT2:
            if (c == '\n')
                state = DEFAULT;
            break;
        case STRING:
            if (c == '"')
                state = DEFAULT;
            else if (c == '\\')
                state = ESCAPE;
            break;
        case ESCAPE:
            state = STRING;
            break;
        case FUNCTION:
            if (c == ' ' || c == '\t' || c == '\n') {
                // ignore
                break;
            } else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
                *token_p++ = c;
                if (token_p == token_limit) {
                    token_p = token;
                    state = DEFAULT;
                }
                continue;
            }
            if (token_p != token) {
                *token_p = '\0';
                hash_set_add(&user_defined_functions, token);
                token_p = token;
            }
            state = DEFAULT;
            goto deflt;
        }
    }
    hash_set_free(&user_defined_functions);
    return ret;
}

int check_compatibility_file_names(int num_files, const char **file_names)
{
    int ret = 0, compat = 0;
    m_cart_memory = malloc(CART_MEMORY_SIZE);
    for (int i=0;i<num_files;++i) {
        const char *lua_script = NULL;
        uint8_t *file_buffer = NULL;

        parse_cart_file(file_names[i], m_cart_memory, &lua_script, &file_buffer);
        if (lua_script) {
            size_t len = strlen(file_names[i]);
            ((char *)file_names[i])[len-3] = 'l';
            ((char *)file_names[i])[len-2] = 'u';
            ((char *)file_names[i]) [len-1] = 'a';
            FILE *fh = fopen(file_names[i], "w");
            fwrite(lua_script, strlen(lua_script), 1, fh);
            fclose(fh);
            int file_ret = check_compatibility(file_names[i], lua_script);
            if (file_ret == 0)
                compat++;
            if (file_ret > ret)
                ret = file_ret;
        }
        if (file_buffer)
            free(file_buffer);
    }
    if (num_files > 1)
        fprintf(stderr, "%d/%d carts are compatible.\n", compat, num_files);
    return ret;
}
