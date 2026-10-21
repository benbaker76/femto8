/**
 * USB HID Scancode Definitions
 *
 * This file defines scancodes according to the USB HID Usage Tables specification.
 * These are the same scancodes used by PICO-8.
 */

#ifndef SCANCODES_H
#define SCANCODES_H

/* Letter keys */
#define SCANCODE_A          4
#define SCANCODE_B          5
#define SCANCODE_C          6
#define SCANCODE_D          7
#define SCANCODE_E          8
#define SCANCODE_F          9
#define SCANCODE_G         10
#define SCANCODE_H         11
#define SCANCODE_I         12
#define SCANCODE_J         13
#define SCANCODE_K         14
#define SCANCODE_L         15
#define SCANCODE_M         16
#define SCANCODE_N         17
#define SCANCODE_O         18
#define SCANCODE_P         19
#define SCANCODE_Q         20
#define SCANCODE_R         21
#define SCANCODE_S         22
#define SCANCODE_T         23
#define SCANCODE_U         24
#define SCANCODE_V         25
#define SCANCODE_W         26
#define SCANCODE_X         27
#define SCANCODE_Y         28
#define SCANCODE_Z         29

/* Number keys */
#define SCANCODE_1         30
#define SCANCODE_2         31
#define SCANCODE_3         32
#define SCANCODE_4         33
#define SCANCODE_5         34
#define SCANCODE_6         35
#define SCANCODE_7         36
#define SCANCODE_8         37
#define SCANCODE_9         38
#define SCANCODE_0         39

/* Other keys */
#define SCANCODE_ENTER     40
#define SCANCODE_ESCAPE    41
#define SCANCODE_BACKSPACE 42
#define SCANCODE_TAB       43
#define SCANCODE_SPACE     44

/* Function keys */
#define SCANCODE_F1        58
#define SCANCODE_F2        59
#define SCANCODE_F3        60
#define SCANCODE_F4        61
#define SCANCODE_F5        62
#define SCANCODE_F6        63
#define SCANCODE_F7        64
#define SCANCODE_F8        65
#define SCANCODE_F9        66
#define SCANCODE_F10       67
#define SCANCODE_F11       68
#define SCANCODE_F12       69

/* Navigation keys */

#define SCANCODE_RIGHT     79
#define SCANCODE_LEFT      80
#define SCANCODE_DOWN      81
#define SCANCODE_UP        82

/* Editing keys */
#define SCANCODE_HOME      74
#define SCANCODE_PAGEUP    75
#define SCANCODE_DEL       76
#define SCANCODE_END       77
#define SCANCODE_PAGEDOWN  78

/* Modifiers */
#define SCANCODE_LEFT_SHIFT   225
#define SCANCODE_RIGHT_SHIFT  229
#define SCANCODE_LEFT_CTRL    224
#define SCANCODE_RIGHT_CTRL   228
#define SCANCODE_LEFT_ALT     226
#define SCANCODE_RIGHT_ALT    230

/* Locking keys */
#define SCANCODE_CAPS_LOCK    57
#define SCANCODE_NUM_LOCK     83
#define SCANCODE_SCROLL_LOCK  71

#endif /* SCANCODES_H */
