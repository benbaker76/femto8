#ifndef P8_SYMBOLS_H
#define P8_SYMBOLS_H

#include <stdint.h>

typedef struct
{
    uint8_t encoding[7];
    uint8_t length;
    uint8_t index;
} p8_symbol_t;

static const p8_symbol_t p8_symbols[] = {
    {{0x5c, 0x30}, 2, 0},
    {{0x5c, 0x2a}, 2, 1},
    {{0x5c, 0x23}, 2, 2},
    {{0x5c, 0x2d}, 2, 3},
    {{0x5c, 0x7c}, 2, 4},
    {{0x5c, 0x2b}, 2, 5},
    {{0x5c, 0x5e}, 2, 6},
    {{0x5c, 0x61}, 2, 7},
    {{0x5c, 0x62}, 2, 8},
    {{0x5c, 0x74}, 2, 9},
    {{0x5c, 0x6e}, 2, 10},
    {{0x5c, 0x76}, 2, 11},
    {{0x5c, 0x66}, 2, 12},
    {{0x5c, 0x72}, 2, 13},
    {{0x5c, 0x30, 0x31, 0x34}, 4, 14},
    {{0x5c, 0x30, 0x31, 0x35}, 4, 15},
    {{0xe2, 0x96, 0xae}, 3, 16},
    {{0xe2, 0x96, 0xa0}, 3, 17},
    {{0xe2, 0x96, 0xa1}, 3, 18},
    {{0xe2, 0x81, 0x99}, 3, 19},
    {{0xe2, 0x81, 0x98}, 3, 20},
    {{0xe2, 0x80, 0x96}, 3, 21},
    {{0xe2, 0x97, 0x80}, 3, 22},
    {{0xe2, 0x96, 0xb6}, 3, 23},
    {{0xe3, 0x80, 0x8c}, 3, 24},
    {{0xe3, 0x80, 0x8d}, 3, 25},
    {{0xc2, 0xa5}, 2, 26},
    {{0xe2, 0x80, 0xa2}, 3, 27},
    {{0xe3, 0x80, 0x81}, 3, 28},
    {{0xe3, 0x80, 0x82}, 3, 29},
    {{0xe3, 0x82, 0x9b}, 3, 30},
    {{0xe3, 0x82, 0x9c}, 3, 31},
    {{0xe2, 0x97, 0x8b}, 3, 127},
    {{0xe2, 0x96, 0x88}, 3, 128},
    {{0xe2, 0x96, 0x92}, 3, 129},
    {{0xf0, 0x9f, 0x90, 0xb1}, 4, 130},
    {{0xe2, 0xac, 0x87, 0xef, 0xb8, 0x8f}, 6, 131},
    {{0xe2, 0x96, 0x91}, 3, 132},
    {{0xe2, 0x9c, 0xbd}, 3, 133},
    {{0xe2, 0x97, 0x8f}, 3, 134},
    {{0xe2, 0x99, 0xa5}, 3, 135},
    {{0xe2, 0x98, 0x89}, 3, 136},
    {{0xec, 0x9b, 0x83}, 3, 137},
    {{0xe2, 0x8c, 0x82}, 3, 138},
    {{0xe2, 0xac, 0x85, 0xef, 0xb8, 0x8f}, 6, 139},
    {{0xf0, 0x9f, 0x98, 0x90}, 4, 140},
    {{0xe2, 0x99, 0xaa}, 3, 141},
    {{0xf0, 0x9f, 0x85, 0xbe, 0xef, 0xb8, 0x8f}, 7, 142},
    {{0xe2, 0x97, 0x86}, 3, 143},
    {{0xe2, 0x80, 0xa6}, 3, 144},
    {{0xe2, 0x9e, 0xa1, 0xef, 0xb8, 0x8f}, 6, 145},
    {{0xe2, 0x98, 0x85}, 3, 146},
    {{0xe2, 0xa7, 0x97}, 3, 147},
    {{0xe2, 0xac, 0x86, 0xef, 0xb8, 0x8f}, 6, 148},
    {{0xcb, 0x87}, 2, 149},
    {{0xe2, 0x88, 0xa7}, 3, 150},
    {{0xe2, 0x9d, 0x8e}, 3, 151},
    {{0xe2, 0x96, 0xa4}, 3, 152},
    {{0xe2, 0x96, 0xa5}, 3, 153},
    {{0xe3, 0x81, 0x82}, 3, 154},
    {{0xe3, 0x81, 0x84}, 3, 155},
    {{0xe3, 0x81, 0x86}, 3, 156},
    {{0xe3, 0x81, 0x88}, 3, 157},
    {{0xe3, 0x81, 0x8a}, 3, 158},
    {{0xe3, 0x81, 0x8b}, 3, 159},
    {{0xe3, 0x81, 0x8d}, 3, 160},
    {{0xe3, 0x81, 0x8f}, 3, 161},
    {{0xe3, 0x81, 0x91}, 3, 162},
    {{0xe3, 0x81, 0x93}, 3, 163},
    {{0xe3, 0x81, 0x95}, 3, 164},
    {{0xe3, 0x81, 0x97}, 3, 165},
    {{0xe3, 0x81, 0x99}, 3, 166},
    {{0xe3, 0x81, 0x9b}, 3, 167},
    {{0xe3, 0x81, 0x9d}, 3, 168},
    {{0xe3, 0x81, 0x9f}, 3, 169},
    {{0xe3, 0x81, 0xa1}, 3, 170},
    {{0xe3, 0x81, 0xa4}, 3, 171},
    {{0xe3, 0x81, 0xa6}, 3, 172},
    {{0xe3, 0x81, 0xa8}, 3, 173},
    {{0xe3, 0x81, 0xaa}, 3, 174},
    {{0xe3, 0x81, 0xab}, 3, 175},
    {{0xe3, 0x81, 0xac}, 3, 176},
    {{0xe3, 0x81, 0xad}, 3, 177},
    {{0xe3, 0x81, 0xae}, 3, 178},
    {{0xe3, 0x81, 0xaf}, 3, 179},
    {{0xe3, 0x81, 0xb2}, 3, 180},
    {{0xe3, 0x81, 0xb5}, 3, 181},
    {{0xe3, 0x81, 0xb8}, 3, 182},
    {{0xe3, 0x81, 0xbb}, 3, 183},
    {{0xe3, 0x81, 0xbe}, 3, 184},
    {{0xe3, 0x81, 0xbf}, 3, 185},
    {{0xe3, 0x82, 0x80}, 3, 186},
    {{0xe3, 0x82, 0x81}, 3, 187},
    {{0xe3, 0x82, 0x82}, 3, 188},
    {{0xe3, 0x82, 0x84}, 3, 189},
    {{0xe3, 0x82, 0x86}, 3, 190},
    {{0xe3, 0x82, 0x88}, 3, 191},
    {{0xe3, 0x82, 0x89}, 3, 192},
    {{0xe3, 0x82, 0x8a}, 3, 193},
    {{0xe3, 0x82, 0x8b}, 3, 194},
    {{0xe3, 0x82, 0x8c}, 3, 195},
    {{0xe3, 0x82, 0x8d}, 3, 196},
    {{0xe3, 0x82, 0x8f}, 3, 197},
    {{0xe3, 0x82, 0x92}, 3, 198},
    {{0xe3, 0x82, 0x93}, 3, 199},
    {{0xe3, 0x81, 0xa3}, 3, 200},
    {{0xe3, 0x82, 0x83}, 3, 201},
    {{0xe3, 0x82, 0x85}, 3, 202},
    {{0xe3, 0x82, 0x87}, 3, 203},
    {{0xe3, 0x82, 0xa2}, 3, 204},
    {{0xe3, 0x82, 0xa4}, 3, 205},
    {{0xe3, 0x82, 0xa6}, 3, 206},
    {{0xe3, 0x82, 0xa8}, 3, 207},
    {{0xe3, 0x82, 0xaa}, 3, 208},
    {{0xe3, 0x82, 0xab}, 3, 209},
    {{0xe3, 0x82, 0xad}, 3, 210},
    {{0xe3, 0x82, 0xaf}, 3, 211},
    {{0xe3, 0x82, 0xb1}, 3, 212},
    {{0xe3, 0x82, 0xb3}, 3, 213},
    {{0xe3, 0x82, 0xb5}, 3, 214},
    {{0xe3, 0x82, 0xb7}, 3, 215},
    {{0xe3, 0x82, 0xb9}, 3, 216},
    {{0xe3, 0x82, 0xbb}, 3, 217},
    {{0xe3, 0x82, 0xbd}, 3, 218},
    {{0xe3, 0x82, 0xbf}, 3, 219},
    {{0xe3, 0x83, 0x81}, 3, 220},
    {{0xe3, 0x83, 0x84}, 3, 221},
    {{0xe3, 0x83, 0x86}, 3, 222},
    {{0xe3, 0x83, 0x88}, 3, 223},
    {{0xe3, 0x83, 0x8a}, 3, 224},
    {{0xe3, 0x83, 0x8b}, 3, 225},
    {{0xe3, 0x83, 0x8c}, 3, 226},
    {{0xe3, 0x83, 0x8d}, 3, 227},
    {{0xe3, 0x83, 0x8e}, 3, 228},
    {{0xe3, 0x83, 0x8f}, 3, 229},
    {{0xe3, 0x83, 0x92}, 3, 230},
    {{0xe3, 0x83, 0x95}, 3, 231},
    {{0xe3, 0x83, 0x98}, 3, 232},
    {{0xe3, 0x83, 0x9b}, 3, 233},
    {{0xe3, 0x83, 0x9e}, 3, 234},
    {{0xe3, 0x83, 0x9f}, 3, 235},
    {{0xe3, 0x83, 0xa0}, 3, 236},
    {{0xe3, 0x83, 0xa1}, 3, 237},
    {{0xe3, 0x83, 0xa2}, 3, 238},
    {{0xe3, 0x83, 0xa4}, 3, 239},
    {{0xe3, 0x83, 0xa6}, 3, 240},
    {{0xe3, 0x83, 0xa8}, 3, 241},
    {{0xe3, 0x83, 0xa9}, 3, 242},
    {{0xe3, 0x83, 0xaa}, 3, 243},
    {{0xe3, 0x83, 0xab}, 3, 244},
    {{0xe3, 0x83, 0xac}, 3, 245},
    {{0xe3, 0x83, 0xad}, 3, 246},
    {{0xe3, 0x83, 0xaf}, 3, 247},
    {{0xe3, 0x83, 0xb2}, 3, 248},
    {{0xe3, 0x83, 0xb3}, 3, 249},
    {{0xe3, 0x83, 0x83}, 3, 250},
    {{0xe3, 0x83, 0xa3}, 3, 251},
    {{0xe3, 0x83, 0xa5}, 3, 252},
    {{0xe3, 0x83, 0xa7}, 3, 253},
    {{0xe2, 0x97, 0x9c}, 3, 254},
    {{0xe2, 0x97, 0x9d}, 3, 255},
};

#endif